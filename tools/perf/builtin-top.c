/*
 * builtin-top.c
 *
 * Builtin top command: Display a continuously updated profile of
 * any workload, CPU or specific PID.
 *
 * Copyright (C) 2008, Red Hat Inc, Ingo Molnar <mingo@redhat.com>
 *
 * Improvements and fixes by:
 *
 *   Arjan van de Ven <arjan@linux.intel.com>
 *   Yanmin Zhang <yanmin.zhang@intel.com>
 *   Wu Fengguang <fengguang.wu@intel.com>
 *   Mike Galbraith <efault@gmx.de>
 *   Paul Mackerras <paulus@samba.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include "builtin.h"

#include "perf.h"

#include "util/color.h"
#include "util/evlist.h"
#include "util/evsel.h"
#include "util/session.h"
#include "util/symbol.h"
#include "util/thread.h"
#include "util/thread_map.h"
#include "util/util.h"
#include <linux/rbtree.h>
#include "util/parse-options.h"
#include "util/parse-events.h"
#include "util/cpumap.h"
#include "util/xyarray.h"

#include "util/debug.h"

#include <assert.h>
#include <fcntl.h>

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>

#include <errno.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>

#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/mman.h>

#include <linux/unistd.h>
#include <linux/types.h>

#define FD(e, x, y) (*(int *)xyarray__entry(e->fd, x, y))

struct perf_evlist		*evsel_list;

static bool			system_wide			=  false;

static int			default_interval		=      0;

static int			count_filter			=      5;
static int			print_entries;

static int			target_pid			=     -1;
static int			target_tid			=     -1;
static struct thread_map	*threads;
static bool			inherit				=  false;
static struct cpu_map		*cpus;
static int			realtime_prio			=      0;
static bool			group				=  false;
static unsigned int		page_size;
static unsigned int		mmap_pages			=    128;
static int			freq				=   1000; /* 1 KHz */

static int			delay_secs			=      2;
static bool			zero                            =  false;
static bool			dump_symtab                     =  false;

static bool			hide_kernel_symbols		=  false;
static bool			hide_user_symbols		=  false;
static struct winsize		winsize;

/*
 * Source
 */

struct source_line {
	u64			eip;
	unsigned long		count[MAX_COUNTERS];
	char			*line;
	struct source_line	*next;
};

static const char		*sym_filter			=   NULL;
struct sym_entry		*sym_filter_entry		=   NULL;
struct sym_entry		*sym_filter_entry_sched		=   NULL;
static int			sym_pcnt_filter			=      5;
static int			sym_counter			=      0;
static struct perf_evsel	*sym_evsel			=   NULL;
static int			display_weighted		=     -1;
static const char		*cpu_list;

/*
 * Symbols
 */

struct sym_entry_source {
	struct source_line	*source;
	struct source_line	*lines;
	struct source_line	**lines_tail;
	pthread_mutex_t		lock;
};

struct sym_entry {
	struct rb_node		rb_node;
	struct list_head	node;
	unsigned long		snap_count;
	double			weight;
	int			skip;
	u16			name_len;
	u8			origin;
	struct map		*map;
	struct sym_entry_source	*src;
	unsigned long		count[0];
};

/*
 * Source functions
 */

static inline struct symbol *sym_entry__symbol(struct sym_entry *self)
{
       return ((void *)self) + symbol_conf.priv_size;
}

void get_term_dimensions(struct winsize *ws)
{
	char *s = getenv("LINES");

	if (s != NULL) {
		ws->ws_row = atoi(s);
		s = getenv("COLUMNS");
		if (s != NULL) {
			ws->ws_col = atoi(s);
			if (ws->ws_row && ws->ws_col)
				return;
		}
	}
#ifdef TIOCGWINSZ
	if (ioctl(1, TIOCGWINSZ, ws) == 0 &&
	    ws->ws_row && ws->ws_col)
		return;
#endif
	ws->ws_row = 25;
	ws->ws_col = 80;
}

static void update_print_entries(struct winsize *ws)
{
	print_entries = ws->ws_row;

	if (print_entries > 9)
		print_entries -= 9;
}

static void sig_winch_handler(int sig __used)
{
	get_term_dimensions(&winsize);
	update_print_entries(&winsize);
}

static int parse_source(struct sym_entry *syme)
{
	struct symbol *sym;
	struct sym_entry_source *source;
	struct map *map;
	FILE *file;
	char command[PATH_MAX*2];
	const char *path;
	u64 len;

	if (!syme)
		return -1;

	sym = sym_entry__symbol(syme);
	map = syme->map;

	/*
	 * We can't annotate with just /proc/kallsyms
	 */
	if (map->dso->origin == DSO__ORIG_KERNEL)
		return -1;

	if (syme->src == NULL) {
		syme->src = zalloc(sizeof(*source));
		if (syme->src == NULL)
			return -1;
		pthread_mutex_init(&syme->src->lock, NULL);
	}

	source = syme->src;

	if (source->lines) {
		pthread_mutex_lock(&source->lock);
		goto out_assign;
	}
	path = map->dso->long_name;

	len = sym->end - sym->start;

	sprintf(command,
		"objdump --start-address=%#0*" PRIx64 " --stop-address=%#0*" PRIx64 " -dS %s",
		BITS_PER_LONG / 4, map__rip_2objdump(map, sym->start),
		BITS_PER_LONG / 4, map__rip_2objdump(map, sym->end), path);

	file = popen(command, "r");
	if (!file)
		return -1;

	pthread_mutex_lock(&source->lock);
	source->lines_tail = &source->lines;
	while (!feof(file)) {
		struct source_line *src;
		size_t dummy = 0;
		char *c, *sep;

		src = malloc(sizeof(struct source_line));
		assert(src != NULL);
		memset(src, 0, sizeof(struct source_line));

		if (getline(&src->line, &dummy, file) < 0)
			break;
		if (!src->line)
			break;

		c = strchr(src->line, '\n');
		if (c)
			*c = 0;

		src->next = NULL;
		*source->lines_tail = src;
		source->lines_tail = &src->next;

		src->eip = strtoull(src->line, &sep, 16);
		if (*sep == ':')
			src->eip = map__objdump_2ip(map, src->eip);
		else /* this line has no ip info (e.g. source line) */
			src->eip = 0;
	}
	pclose(file);
out_assign:
	sym_filter_entry = syme;
	pthread_mutex_unlock(&source->lock);
	return 0;
}

static void __zero_source_counters(struct sym_entry *syme)
{
	int i;
	struct source_line *line;

	line = syme->src->lines;
	while (line) {
		for (i = 0; i < evsel_list->nr_entries; i++)
			line->count[i] = 0;
		line = line->next;
	}
}

static void record_precise_ip(struct sym_entry *syme, int counter, u64 ip)
{
	struct source_line *line;

	if (syme != sym_filter_entry)
		return;

	if (pthread_mutex_trylock(&syme->src->lock))
		return;

	if (syme->src == NULL || syme->src->source == NULL)
		goto out_unlock;

	for (line = syme->src->lines; line; line = line->next) {
		/* skip lines without IP info */
		if (line->eip == 0)
			continue;
		if (line->eip == ip) {
			line->count[counter]++;
			break;
		}
		if (line->eip > ip)
			break;
	}
out_unlock:
	pthread_mutex_unlock(&syme->src->lock);
}

#define PATTERN_LEN		(BITS_PER_LONG / 4 + 2)

static void lookup_sym_source(struct sym_entry *syme)
{
	struct symbol *symbol = sym_entry__symbol(syme);
	struct source_line *line;
	char pattern[PATTERN_LEN + 1];

	sprintf(pattern, "%0*" PRIx64 " <", BITS_PER_LONG / 4,
		map__rip_2objdump(syme->map, symbol->start));

	pthread_mutex_lock(&syme->src->lock);
	for (line = syme->src->lines; line; line = line->next) {
		if (memcmp(line->line, pattern, PATTERN_LEN) == 0) {
			syme->src->source = line;
			break;
		}
	}
	pthread_mutex_unlock(&syme->src->lock);
}

static void show_lines(struct source_line *queue, int count, int total)
{
	int i;
	struct source_line *line;

	line = queue;
	for (i = 0; i < count; i++) {
		float pcnt = 100.0*(float)line->count[sym_counter]/(float)total;

		printf("%8li %4.1f%%\t%s\n", line->count[sym_counter], pcnt, line->line);
		line = line->next;
	}
}

#define TRACE_COUNT     3

static void show_details(struct sym_entry *syme)
{
	struct symbol *symbol;
	struct source_line *line;
	struct source_line *line_queue = NULL;
	int displayed = 0;
	int line_queue_count = 0, total = 0, more = 0;

	if (!syme)
		return;

	if (!syme->src->source)
		lookup_sym_source(syme);

	if (!syme->src->source)
		return;

	symbol = sym_entry__symbol(syme);
	printf("Showing %s for %s\n", event_name(sym_evsel), symbol->name);
	printf("  Events  Pcnt (>=%d%%)\n", sym_pcnt_filter);

	pthread_mutex_lock(&syme->src->lock);
	line = syme->src->source;
	while (line) {
		total += line->count[sym_counter];
		line = line->next;
	}

	line = syme->src->source;
	while (line) {
		float pcnt = 0.0;

		if (!line_queue_count)
			line_queue = line;
		line_queue_count++;

		if (line->count[sym_counter])
			pcnt = 100.0 * line->count[sym_counter] / (float)total;
		if (pcnt >= (float)sym_pcnt_filter) {
			if (displayed <= print_entries)
				show_lines(line_queue, line_queue_count, total);
			else more++;
			displayed += line_queue_count;
			line_queue_count = 0;
			line_queue = NULL;
		} else if (line_queue_count > TRACE_COUNT) {
			line_queue = line_queue->next;
			line_queue_count--;
		}

		line->count[sym_counter] = zero ? 0 : line->count[sym_counter] * 7 / 8;
		line = line->next;
	}
	pthread_mutex_unlock(&syme->src->lock);
	if (more)
		printf("%d lines not displayed, maybe increase display entries [e]\n", more);
}

/*
 * Symbols will be added here in event__process_sample and will get out
 * after decayed.
 */
static LIST_HEAD(active_symbols);
static pthread_mutex_t active_symbols_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Ordering weight: count-1 * count-2 * ... / count-n
 */
static double sym_weight(const struct sym_entry *sym)
{
	double weight = sym->snap_count;
	int counter;

	if (!display_weighted)
		return weight;

	for (counter = 1; counter < evsel_list->nr_entries - 1; counter++)
		weight *= sym->count[counter];

	weight /= (sym->count[counter] + 1);

	return weight;
}

static long			samples;
static long			kernel_samples, us_samples;
static long			exact_samples;
static long			guest_us_samples, guest_kernel_samples;
static const char		CONSOLE_CLEAR[] = "[H[2J";

static void __list_insert_active_sym(struct sym_entry *syme)
{
	list_add(&syme->node, &active_symbols);
}

static void list_remove_active_sym(struct sym_entry *syme)
{
	pthread_mutex_lock(&active_symbols_lock);
	list_del_init(&syme->node);
	pthread_mutex_unlock(&active_symbols_lock);
}

static void rb_insert_active_sym(struct rb_root *tree, struct sym_entry *se)
{
	struct rb_node **p = &tree->rb_node;
	struct rb_node *parent = NULL;
	struct sym_entry *iter;

	while (*p != NULL) {
		parent = *p;
		iter = rb_entry(parent, struct sym_entry, rb_node);

		if (se->weight > iter->weight)
			p = &(*p)->rb_left;
		else
			p = &(*p)->rb_right;
	}

	rb_link_node(&se->rb_node, parent, p);
	rb_insert_color(&se->rb_node, tree);
}

static void print_sym_table(struct perf_session *session)
{
	int printed = 0, j;
	struct perf_evsel *counter;
	int snap = !display_weighted ? sym_counter : 0;
	float samples_per_sec = samples/delay_secs;
	float ksamples_per_sec = kernel_samples/delay_secs;
	float us_samples_per_sec = (us_samples)/delay_secs;
	float guest_kernel_samples_per_sec = (guest_kernel_samples)/delay_secs;
	float guest_us_samples_per_sec = (guest_us_samples)/delay_secs;
	float esamples_percent = (100.0*exact_samples)/samples;
	float sum_ksamples = 0.0;
	struct sym_entry *syme, *n;
	struct rb_root tmp = RB_ROOT;
	struct rb_node *nd;
	int sym_width = 0, dso_width = 0, dso_short_width = 0;
	const int win_width = winsize.ws_col - 1;

	samples = us_samples = kernel_samples = exact_samples = 0;
	guest_kernel_samples = guest_us_samples = 0;

	/* Sort the active symbols */
	pthread_mutex_lock(&active_symbols_lock);
	syme = list_entry(active_symbols.next, struct sym_entry, node);
	pthread_mutex_unlock(&active_symbols_lock);

	list_for_each_entry_safe_from(syme, n, &active_symbols, node) {
		syme->snap_count = syme->count[snap];
		if (syme->snap_count != 0) {

			if ((hide_user_symbols &&
			     syme->origin == PERF_RECORD_MISC_USER) ||
			    (hide_kernel_symbols &&
			     syme->origin == PERF_RECORD_MISC_KERNEL)) {
				list_remove_active_sym(syme);
				continue;
			}
			syme->weight = sym_weight(syme);
			rb_insert_active_sym(&tmp, syme);
			sum_ksamples += syme->snap_count;

			for (j = 0; j < evsel_list->nr_entries; j++)
				syme->count[j] = zero ? 0 : syme->count[j] * 7 / 8;
		} else
			list_remove_active_sym(syme);
	}

	puts(CONSOLE_CLEAR);

	if (!perf_guest) {
		printf("   PerfTop:%8.0f irqs/sec  kernel:%4.1f%%"
			"  exact: %4.1f%% [",
			samples_per_sec,
			100.0 - (100.0 * ((samples_per_sec - ksamples_per_sec) /
					 samples_per_sec)),
			esamples_percent);
	} else {
		printf("   PerfTop:%8.0f irqs/sec  kernel:%4.1f%% us:%4.1f%%"
			" guest kernel:%4.1f%% guest us:%4.1f%%"
			" exact: %4.1f%% [",
			samples_per_sec,
			100.0 - (100.0 * ((samples_per_sec-ksamples_per_sec) /
					  samples_per_sec)),
			100.0 - (100.0 * ((samples_per_sec-us_samples_per_sec) /
					  samples_per_sec)),
			100.0 - (100.0 * ((samples_per_sec -
						guest_kernel_samples_per_sec) /
					  samples_per_sec)),
			100.0 - (100.0 * ((samples_per_sec -
					   guest_us_samples_per_sec) /
					  samples_per_sec)),
			esamples_percent);
	}

	if (evsel_list->nr_entries == 1 || !display_weighted) {
		struct perf_evsel *first;
		first = list_entry(evsel_list->entries.next, struct perf_evsel, node);
		printf("%" PRIu64, (uint64_t)first->attr.sample_period);
		if (freq)
			printf("Hz ");
		else
			printf(" ");
	}

	if (!display_weighted)
		printf("%s", event_name(sym_evsel));
	else list_for_each_entry(counter, &evsel_list->entries, node) {
		if (counter->idx)
			printf("/");

		printf("%s", event_name(counter));
	}

	printf( "], ");

	if (target_pid != -1)
		printf(" (target_pid: %d", target_pid);
	else if (target_tid != -1)
		printf(" (target_tid: %d", target_tid);
	else
		printf(" (all");

	if (cpu_list)
		printf(", CPU%s: %s)\n", cpus->nr > 1 ? "s" : "", cpu_list);
	else {
		if (target_tid != -1)
			printf(")\n");
		else
			printf(", %d CPU%s)\n", cpus->nr, cpus->nr > 1 ? "s" : "");
	}

	printf("%-*.*s\n", win_width, win_width, graph_dotted_line);

	if (session->hists.stats.total_lost != 0) {
		color_fprintf(stdout, PERF_COLOR_RED, "WARNING:");
		printf(" LOST %" PRIu64 " events, Check IO/CPU overload\n",
		       session->hists.stats.total_lost);
	}

	if (sym_filter_entry) {
		show_details(sym_filter_entry);
		return;
	}

	/*
	 * Find the longest symbol name that will be displayed
	 */
	for (nd = rb_first(&tmp); nd; nd = rb_next(nd)) {
		syme = rb_entry(nd, struct sym_entry, rb_node);
		if (++printed > print_entries ||
		    (int)syme->snap_count < count_filter)
			continue;

		if (syme->map->dso->long_name_len > dso_width)
			dso_width = syme->map->dso->long_name_len;

		if (syme->map->dso->short_name_len > dso_short_width)
			dso_short_width = syme->map->dso->short_name_len;

		if (syme->name_len > sym_width)
			sym_width = syme->name_len;
	}

	printed = 0;

	if (sym_width + dso_width > winsize.ws_col - 29) {
		dso_width = dso_short_width;
		if (sym_width + dso_width > winsize.ws_col - 29)
			sym_width = winsize.ws_col - dso_width - 29;
	}
	putchar('\n');
	if (evsel_list->nr_entries == 1)
		printf("             samples  pcnt");
	else
		printf("   weight    samples  pcnt");

	if (verbose)
		printf("         RIP       ");
	printf(" %-*.*s DSO\n", sym_width, sym_width, "function");
	printf("   %s    _______ _____",
	       evsel_list->nr_entries == 1 ? "      " : "______");
	if (verbose)
		printf(" ________________");
	printf(" %-*.*s", sym_width, sym_width, graph_line);
	printf(" %-*.*s", dso_width, dso_width, graph_line);
	puts("\n");

	for (nd = rb_first(&tmp); nd; nd = rb_next(nd)) {
		struct symbol *sym;
		double pcnt;

		syme = rb_entry(nd, struct sym_entry, rb_node);
		sym = sym_entry__symbol(syme);
		if (++printed > print_entries || (int)syme->snap_count < count_filter)
			continue;

		pcnt = 100.0 - (100.0 * ((sum_ksamples - syme->snap_count) /
					 sum_ksamples));

		if (evsel_list->nr_entries == 1 || !display_weighted)
			printf("%20.2f ", syme->weight);
		else
			printf("%9.1f %10ld ", syme->weight, syme->snap_count);

		percent_color_fprintf(stdout, "%4.1f%%", pcnt);
		if (verbose)
			printf(" %016" PRIx64, sym->start);
		printf(" %-*.*s", sym_width, sym_width, sym->name);
		printf(" %-*.*s\n", dso_width, dso_width,
		       dso_width >= syme->map->dso->long_name_len ?
					syme->map->dso->long_name :
					syme->map->dso->short_name);
	}
}

static void prompt_integer(int *target, const char *msg)
{
	char *buf = malloc(0), *p;
	size_t dummy = 0;
	int tmp;

	fprintf(stdout, "\n%s: ", msg);
	if (getline(&buf, &dummy, stdin) < 0)
		return;

	p = strchr(buf, '\n');
	if (p)
		*p = 0;

	p = buf;
	while(*p) {
		if (!isdigit(*p))
			goto out_free;
		p++;
	}
	tmp = strtoul(buf, NULL, 10);
	*target = tmp;
out_free:
	free(buf);
}

static void prompt_percent(int *target, const char *msg)
{
	int tmp = 0;

	prompt_integer(&tmp, msg);
	if (tmp >= 0 && tmp <= 100)
		*target = tmp;
}

static void prompt_symbol(struct sym_entry **target, const char *msg)
{
	char *buf = malloc(0), *p;
	struct sym_entry *syme = *target, *n, *found = NULL;
	size_t dummy = 0;

	/* zero counters of active symbol */
	if (syme) {
		pthread_mutex_lock(&syme->src->lock);
		__zero_source_counters(syme);
		*target = NULL;
		pthread_mutex_unlock(&syme->src->lock);
	}

	fprintf(stdout, "\n%s: ", msg);
	if (getline(&buf, &dummy, stdin) < 0)
		goto out_free;

	p = strchr(buf, '\n');
	if (p)
		*p = 0;

	pthread_mutex_lock(&active_symbols_lock);
	syme = list_entry(active_symbols.next, struct sym_entry, node);
	pthread_mutex_unlock(&active_symbols_lock);

	list_for_each_entry_safe_from(syme, n, &active_symbols, node) {
		struct symbol *sym = sym_entry__symbol(syme);

		if (!strcmp(buf, sym->name)) {
			found = syme;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "Sorry, %s is not active.\n", buf);
		sleep(1);
		return;
	} else
		parse_source(found);

out_free:
	free(buf);
}

static void print_mapped_keys(void)
{
	char *name = NULL;

	if (sym_filter_entry) {
		struct symbol *sym = sym_entry__symbol(sym_filter_entry);
		name = sym->name;
	}

	fprintf(stdout, "\nMapped keys:\n");
	fprintf(stdout, "\t[d]     display refresh delay.             \t(%d)\n", delay_secs);
	fprintf(stdout, "\t[e]     display entries (lines).           \t(%d)\n", print_entries);

	if (evsel_list->nr_entries > 1)
		fprintf(stdout, "\t[E]     active event counter.              \t(%s)\n", event_name(sym_evsel));

	fprintf(stdout, "\t[f]     profile display filter (count).    \t(%d)\n", count_filter);

	fprintf(stdout, "\t[F]     annotate display filter (percent). \t(%d%%)\n", sym_pcnt_filter);
	fprintf(stdout, "\t[s]     annotate symbol.                   \t(%s)\n", name?: "NULL");
	fprintf(stdout, "\t[S]     stop annotation.\n");

	if (evsel_list->nr_entries > 1)
		fprintf(stdout, "\t[w]     toggle display weighted/count[E]r. \t(%d)\n", display_weighted ? 1 : 0);

	fprintf(stdout,
		"\t[K]     hide kernel_symbols symbols.     \t(%s)\n",
		hide_kernel_symbols ? "yes" : "no");
	fprintf(stdout,
		"\t[U]     hide user symbols.               \t(%s)\n",
		hide_user_symbols ? "yes" : "no");
	fprintf(stdout, "\t[z]     toggle sample zeroing.             \t(%d)\n", zero ? 1 : 0);
	fprintf(stdout, "\t[qQ]    quit.\n");
}

static int key_mapped(int c)
{
	switch (c) {
		case 'd':
		case 'e':
		case 'f':
		case 'z':
		case 'q':
		case 'Q':
		case 'K':
		case 'U':
		case 'F':
		case 's':
		case 'S':
			return 1;
		case 'E':
		case 'w':
			return evsel_list->nr_entries > 1 ? 1 : 0;
		default:
			break;
	}

	return 0;
}

static void handle_keypress(struct perf_session *session, int c)
{
	if (!key_mapped(c)) {
		struct pollfd stdin_poll = { .fd = 0, .events = POLLIN };
		struct termios tc, save;

		print_mapped_keys();
		fprintf(stdout, "\nEnter selection, or unmapped key to continue: ");
		fflush(stdout);

		tcgetattr(0, &save);
		tc = save;
		tc.c_lflag &= ~(ICANON | ECHO);
		tc.c_cc[VMIN] = 0;
		tc.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &tc);

		poll(&stdin_poll, 1, -1);
		c = getc(stdin);

		tcsetattr(0, TCSAFLUSH, &save);
		if (!key_mapped(c))
			return;
	}

	switch (c) {
		case 'd':
			prompt_integer(&delay_secs, "Enter display delay");
			if (delay_secs < 1)
				delay_secs = 1;
			break;
		case 'e':
			prompt_integer(&print_entries, "Enter display entries (lines)");
			if (print_entries == 0) {
				sig_winch_handler(SIGWINCH);
				signal(SIGWINCH, sig_winch_handler);
			} else
				signal(SIGWINCH, SIG_DFL);
			break;
		case 'E':
			if (evsel_list->nr_entries > 1) {
				fprintf(stderr, "\nAvailable events:");

				list_for_each_entry(sym_evsel, &evsel_list->entries, node)
					fprintf(stderr, "\n\t%d %s", sym_evsel->idx, event_name(sym_evsel));

				prompt_integer(&sym_counter, "Enter details event counter");

				if (sym_counter >= evsel_list->nr_entries) {
					sym_evsel = list_entry(evsel_list->entries.next, struct perf_evsel, node);
					sym_counter = 0;
					fprintf(stderr, "Sorry, no such event, using %s.\n", event_name(sym_evsel));
					sleep(1);
					break;
				}
				list_for_each_entry(sym_evsel, &evsel_list->entries, node)
					if (sym_evsel->idx == sym_counter)
						break;
			} else sym_counter = 0;
			break;
		case 'f':
			prompt_integer(&count_filter, "Enter display event count filter");
			break;
		case 'F':
			prompt_percent(&sym_pcnt_filter, "Enter details display event filter (percent)");
			break;
		case 'K':
			hide_kernel_symbols = !hide_kernel_symbols;
			break;
		case 'q':
		case 'Q':
			printf("exiting.\n");
			if (dump_symtab)
				perf_session__fprintf_dsos(session, stderr);
			exit(0);
		case 's':
			prompt_symbol(&sym_filter_entry, "Enter details symbol");
			break;
		case 'S':
			if (!sym_filter_entry)
				break;
			else {
				struct sym_entry *syme = sym_filter_entry;

				pthread_mutex_lock(&syme->src->lock);
				sym_filter_entry = NULL;
				__zero_source_counters(syme);
				pthread_mutex_unlock(&syme->src->lock);
			}
			break;
		case 'U':
			hide_user_symbols = !hide_user_symbols;
			break;
		case 'w':
			display_weighted = ~display_weighted;
			break;
		case 'z':
			zero = !zero;
			break;
		default:
			break;
	}
}

static void *display_thread(void *arg __used)
{
	struct pollfd stdin_poll = { .fd = 0, .events = POLLIN };
	struct termios tc, save;
	int delay_msecs, c;
	struct perf_session *session = (struct perf_session *) arg;

	tcgetattr(0, &save);
	tc = save;
	tc.c_lflag &= ~(ICANON | ECHO);
	tc.c_cc[VMIN] = 0;
	tc.c_cc[VTIME] = 0;

repeat:
	delay_msecs = delay_secs * 1000;
	tcsetattr(0, TCSANOW, &tc);
	/* trash return*/
	getc(stdin);

	do {
		print_sym_table(session);
	} while (!poll(&stdin_poll, 1, delay_msecs) == 1);

	c = getc(stdin);
	tcsetattr(0, TCSAFLUSH, &save);

	handle_keypress(session, c);
	goto repeat;

	return NULL;
}

/* Tag samples to be skipped. */
static const char *skip_symbols[] = {
	"default_idle",
	"native_safe_halt",
	"cpu_idle",
	"enter_idle",
	"exit_idle",
	"mwait_idle",
	"mwait_idle_with_hints",
	"poll_idle",
	"ppc64_runlatch_off",
	"pseries_dedicated_idle_sleep",
	NULL
};

static int symbol_filter(struct map *map, struct symbol *sym)
{
	struct sym_entry *syme;
	const char *name = sym->name;
	int i;

	/*
	 * ppc64 uses function descriptors and appends a '.' to the
	 * start of every instruction address. Remove it.
	 */
	if (name[0] == '.')
		name++;

	if (!strcmp(name, "_text") ||
	    !strcmp(name, "_etext") ||
	    !strcmp(name, "_sinittext") ||
	    !strncmp("init_module", name, 11) ||
	    !strncmp("cleanup_module", name, 14) ||
	    strstr(name, "_text_start") ||
	    strstr(name, "_text_end"))
		return 1;

	syme = symbol__priv(sym);
	syme->map = map;
	syme->src = NULL;

	if (!sym_filter_entry && sym_filter && !strcmp(name, sym_filter)) {
		/* schedule initial sym_filter_entry setup */
		sym_filter_entry_sched = syme;
		sym_filter = NULL;
	}

	for (i = 0; skip_symbols[i]; i++) {
		if (!strcmp(skip_symbols[i], name)) {
			syme->skip = 1;
			break;
		}
	}

	if (!syme->skip)
		syme->name_len = strlen(sym->name);

	return 0;
}

static void event__process_sample(const event_t *self,
				  struct perf_sample *sample,
				  struct perf_session *session)
{
	u64 ip = self->ip.ip;
	struct sym_entry *syme;
	struct addr_location al;
	struct machine *machine;
	u8 origin = self->header.misc & PERF_RECORD_MISC_CPUMODE_MASK;

	++samples;

	switch (origin) {
	case PERF_RECORD_MISC_USER:
		++us_samples;
		if (hide_user_symbols)
			return;
		machine = perf_session__find_host_machine(session);
		break;
	case PERF_RECORD_MISC_KERNEL:
		++kernel_samples;
		if (hide_kernel_symbols)
			return;
		machine = perf_session__find_host_machine(session);
		break;
	case PERF_RECORD_MISC_GUEST_KERNEL:
		++guest_kernel_samples;
		machine = perf_session__find_machine(session, self->ip.pid);
		break;
	case PERF_RECORD_MISC_GUEST_USER:
		++guest_us_samples;
		/*
		 * TODO: we don't process guest user from host side
		 * except simple counting.
		 */
		return;
	default:
		return;
	}

	if (!machine && perf_guest) {
		pr_err("Can't find guest [%d]'s kernel information\n",
			self->ip.pid);
		return;
	}

	if (self->header.misc & PERF_RECORD_MISC_EXACT_IP)
		exact_samples++;

	if (event__preprocess_sample(self, session, &al, sample,
				     symbol_filter) < 0 ||
	    al.filtered)
		return;

	if (al.sym == NULL) {
		/*
		 * As we do lazy loading of symtabs we only will know if the
		 * specified vmlinux file is invalid when we actually have a
		 * hit in kernel space and then try to load it. So if we get
		 * here and there are _no_ symbols in the DSO backing the
		 * kernel map, bail out.
		 *
		 * We may never get here, for instance, if we use -K/
		 * --hide-kernel-symbols, even if the user specifies an
		 * invalid --vmlinux ;-)
		 */
		if (al.map == machine->vmlinux_maps[MAP__FUNCTION] &&
		    RB_EMPTY_ROOT(&al.map->dso->symbols[MAP__FUNCTION])) {
			pr_err("The %s file can't be used\n",
			       symbol_conf.vmlinux_name);
			exit(1);
		}

		return;
	}

	/* let's see, whether we need to install initial sym_filter_entry */
	if (sym_filter_entry_sched) {
		sym_filter_entry = sym_filter_entry_sched;
		sym_filter_entry_sched = NULL;
		if (parse_source(sym_filter_entry) < 0) {
			struct symbol *sym = sym_entry__symbol(sym_filter_entry);

			pr_err("Can't annotate %s", sym->name);
			if (sym_filter_entry->map->dso->origin == DSO__ORIG_KERNEL) {
				pr_err(": No vmlinux file was found in the path:\n");
				machine__fprintf_vmlinux_path(machine, stderr);
			} else
				pr_err(".\n");
			exit(1);
		}
	}

	syme = symbol__priv(al.sym);
	if (!syme->skip) {
		struct perf_evsel *evsel;

		syme->origin = origin;
		evsel = perf_evlist__id2evsel(evsel_list, sample->id);
		assert(evsel != NULL);
		syme->count[evsel->idx]++;
		record_precise_ip(syme, evsel->idx, ip);
		pthread_mutex_lock(&active_symbols_lock);
		if (list_empty(&syme->node) || !syme->node.next)
			__list_insert_active_sym(syme);
		pthread_mutex_unlock(&active_symbols_lock);
	}
}

static void perf_session__mmap_read_cpu(struct perf_session *self, int cpu)
{
	struct perf_sample sample;
	event_t *event;

	while ((event = perf_evlist__read_on_cpu(evsel_list, cpu)) != NULL) {
		perf_session__parse_sample(self, event, &sample);

		if (event->header.type == PERF_RECORD_SAMPLE)
			event__process_sample(event, &sample, self);
		else
			event__process(event, &sample, self);
	}
}

static void perf_session__mmap_read(struct perf_session *self)
{
	int i;

	for (i = 0; i < cpus->nr; i++)
		perf_session__mmap_read_cpu(self, i);
}

static void start_counters(struct perf_evlist *evlist)
{
	struct perf_evsel *counter;

	list_for_each_entry(counter, &evlist->entries, node) {
		struct perf_event_attr *attr = &counter->attr;

		attr->sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID;

		if (freq) {
			attr->sample_type |= PERF_SAMPLE_PERIOD;
			attr->freq	  = 1;
			attr->sample_freq = freq;
		}

		if (evlist->nr_entries > 1) {
			attr->sample_type |= PERF_SAMPLE_ID;
			attr->read_format |= PERF_FORMAT_ID;
		}

		attr->mmap = 1;
try_again:
		if (perf_evsel__open(counter, cpus, threads, group, inherit) < 0) {
			int err = errno;

			if (err == EPERM || err == EACCES)
				die("Permission error - are you root?\n"
					"\t Consider tweaking"
					" /proc/sys/kernel/perf_event_paranoid.\n");
			/*
			 * If it's cycles then fall back to hrtimer
			 * based cpu-clock-tick sw counter, which
			 * is always available even if no PMU support:
			 */
			if (attr->type == PERF_TYPE_HARDWARE &&
			    attr->config == PERF_COUNT_HW_CPU_CYCLES) {

				if (verbose)
					warning(" ... trying to fall back to cpu-clock-ticks\n");

				attr->type = PERF_TYPE_SOFTWARE;
				attr->config = PERF_COUNT_SW_CPU_CLOCK;
				goto try_again;
			}
			printf("\n");
			error("sys_perf_event_open() syscall returned with %d "
			      "(%s).  /bin/dmesg may provide additional information.\n",
			      err, strerror(err));
			die("No CONFIG_PERF_EVENTS=y kernel support configured?\n");
			exit(-1);
		}
	}

	if (perf_evlist__mmap(evlist, cpus, threads, mmap_pages, false) < 0)
		die("failed to mmap with %d (%s)\n", errno, strerror(errno));
}

static int __cmd_top(void)
{
	pthread_t thread;
	struct perf_evsel *first;
	int ret;
	/*
	 * FIXME: perf_session__new should allow passing a O_MMAP, so that all this
	 * mmap reading, etc is encapsulated in it. Use O_WRONLY for now.
	 */
	struct perf_session *session = perf_session__new(NULL, O_WRONLY, false, false, NULL);
	if (session == NULL)
		return -ENOMEM;

	if (target_tid != -1)
		event__synthesize_thread(target_tid, event__process, session);
	else
		event__synthesize_threads(event__process, session);

	start_counters(evsel_list);
	first = list_entry(evsel_list->entries.next, struct perf_evsel, node);
	perf_session__set_sample_type(session, first->attr.sample_type);

	/* Wait for a minimal set of events before starting the snapshot */
	poll(evsel_list->pollfd, evsel_list->nr_fds, 100);

	perf_session__mmap_read(session);

	if (pthread_create(&thread, NULL, display_thread, session)) {
		printf("Could not create display thread.\n");
		exit(-1);
	}

	if (realtime_prio) {
		struct sched_param param;

		param.sched_priority = realtime_prio;
		if (sched_setscheduler(0, SCHED_FIFO, &param)) {
			printf("Could not set realtime priority.\n");
			exit(-1);
		}
	}

	while (1) {
		int hits = samples;

		perf_session__mmap_read(session);

		if (hits == samples)
			ret = poll(evsel_list->pollfd, evsel_list->nr_fds, 100);
	}

	return 0;
}

static const char * const top_usage[] = {
	"perf top [<options>]",
	NULL
};

static const struct option options[] = {
	OPT_CALLBACK('e', "event", &evsel_list, "event",
		     "event selector. use 'perf list' to list available events",
		     parse_events),
	OPT_INTEGER('c', "count", &default_interval,
		    "event period to sample"),
	OPT_INTEGER('p', "pid", &target_pid,
		    "profile events on existing process id"),
	OPT_INTEGER('t', "tid", &target_tid,
		    "profile events on existing thread id"),
	OPT_BOOLEAN('a', "all-cpus", &system_wide,
			    "system-wide collection from all CPUs"),
	OPT_STRING('C', "cpu", &cpu_list, "cpu",
		    "list of cpus to monitor"),
	OPT_STRING('k', "vmlinux", &symbol_conf.vmlinux_name,
		   "file", "vmlinux pathname"),
	OPT_BOOLEAN('K', "hide_kernel_symbols", &hide_kernel_symbols,
		    "hide kernel symbols"),
	OPT_UINTEGER('m', "mmap-pages", &mmap_pages, "number of mmap data pages"),
	OPT_INTEGER('r', "realtime", &realtime_prio,
		    "collect data with this RT SCHED_FIFO priority"),
	OPT_INTEGER('d', "delay", &delay_secs,
		    "number of seconds to delay between refreshes"),
	OPT_BOOLEAN('D', "dump-symtab", &dump_symtab,
			    "dump the symbol table used for profiling"),
	OPT_INTEGER('f', "count-filter", &count_filter,
		    "only display functions with more events than this"),
	OPT_BOOLEAN('g', "group", &group,
			    "put the counters into a counter group"),
	OPT_BOOLEAN('i', "inherit", &inherit,
		    "child tasks inherit counters"),
	OPT_STRING('s', "sym-annotate", &sym_filter, "symbol name",
		    "symbol to annotate"),
	OPT_BOOLEAN('z', "zero", &zero,
		    "zero history across updates"),
	OPT_INTEGER('F', "freq", &freq,
		    "profile at this frequency"),
	OPT_INTEGER('E', "entries", &print_entries,
		    "display this many functions"),
	OPT_BOOLEAN('U', "hide_user_symbols", &hide_user_symbols,
		    "hide user symbols"),
	OPT_INCR('v', "verbose", &verbose,
		    "be more verbose (show counter open errors, etc)"),
	OPT_END()
};

int cmd_top(int argc, const char **argv, const char *prefix __used)
{
	struct perf_evsel *pos;
	int status = -ENOMEM;

	evsel_list = perf_evlist__new();
	if (evsel_list == NULL)
		return -ENOMEM;

	page_size = sysconf(_SC_PAGE_SIZE);

	argc = parse_options(argc, argv, options, top_usage, 0);
	if (argc)
		usage_with_options(top_usage, options);

	if (target_pid != -1)
		target_tid = target_pid;

	threads = thread_map__new(target_pid, target_tid);
	if (threads == NULL) {
		pr_err("Problems finding threads of monitor\n");
		usage_with_options(top_usage, options);
	}

	/* CPU and PID are mutually exclusive */
	if (target_tid > 0 && cpu_list) {
		printf("WARNING: PID switch overriding CPU\n");
		sleep(1);
		cpu_list = NULL;
	}

	if (!evsel_list->nr_entries &&
	    perf_evlist__add_default(evsel_list) < 0) {
		pr_err("Not enough memory for event selector list\n");
		return -ENOMEM;
	}

	if (delay_secs < 1)
		delay_secs = 1;

	/*
	 * User specified count overrides default frequency.
	 */
	if (default_interval)
		freq = 0;
	else if (freq) {
		default_interval = freq;
	} else {
		fprintf(stderr, "frequency and count are zero, aborting\n");
		exit(EXIT_FAILURE);
	}

	if (target_tid != -1)
		cpus = cpu_map__dummy_new();
	else
		cpus = cpu_map__new(cpu_list);

	if (cpus == NULL)
		usage_with_options(top_usage, options);

	list_for_each_entry(pos, &evsel_list->entries, node) {
		if (perf_evsel__alloc_fd(pos, cpus->nr, threads->nr) < 0)
			goto out_free_fd;
		/*
		 * Fill in the ones not specifically initialized via -c:
		 */
		if (pos->attr.sample_period)
			continue;

		pos->attr.sample_period = default_interval;
	}

	if (perf_evlist__alloc_pollfd(evsel_list, cpus->nr, threads->nr) < 0 ||
	    perf_evlist__alloc_mmap(evsel_list, cpus->nr) < 0)
		goto out_free_fd;

	sym_evsel = list_entry(evsel_list->entries.next, struct perf_evsel, node);

	symbol_conf.priv_size = (sizeof(struct sym_entry) +
				 (evsel_list->nr_entries + 1) * sizeof(unsigned long));

	symbol_conf.try_vmlinux_path = (symbol_conf.vmlinux_name == NULL);
	if (symbol__init() < 0)
		return -1;

	get_term_dimensions(&winsize);
	if (print_entries == 0) {
		update_print_entries(&winsize);
		signal(SIGWINCH, sig_winch_handler);
	}

	status = __cmd_top();
out_free_fd:
	perf_evlist__delete(evsel_list);

	return status;
}
