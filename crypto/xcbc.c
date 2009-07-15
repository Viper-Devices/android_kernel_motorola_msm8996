/*
 * Copyright (C)2006 USAGI/WIDE Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:
 * 	Kazunori Miyazawa <miyazawa@linux-ipv6.org>
 */

#include <crypto/internal/hash.h>
#include <linux/err.h>
#include <linux/kernel.h>

static u_int32_t ks[12] = {0x01010101, 0x01010101, 0x01010101, 0x01010101,
			   0x02020202, 0x02020202, 0x02020202, 0x02020202,
			   0x03030303, 0x03030303, 0x03030303, 0x03030303};
/*
 * +------------------------
 * | <parent tfm>
 * +------------------------
 * | crypto_xcbc_ctx
 * +------------------------
 * | odds (block size)
 * +------------------------
 * | prev (block size)
 * +------------------------
 * | key (block size)
 * +------------------------
 * | consts (block size * 3)
 * +------------------------
 */
struct crypto_xcbc_ctx {
	struct crypto_cipher *child;
	u8 *odds;
	u8 *prev;
	u8 *key;
	u8 *consts;
	void (*xor)(u8 *a, const u8 *b, unsigned int bs);
	unsigned int keylen;
	unsigned int len;
};

static void xor_128(u8 *a, const u8 *b, unsigned int bs)
{
	((u32 *)a)[0] ^= ((u32 *)b)[0];
	((u32 *)a)[1] ^= ((u32 *)b)[1];
	((u32 *)a)[2] ^= ((u32 *)b)[2];
	((u32 *)a)[3] ^= ((u32 *)b)[3];
}

static int _crypto_xcbc_digest_setkey(struct crypto_shash *parent,
				      struct crypto_xcbc_ctx *ctx)
{
	int bs = crypto_shash_blocksize(parent);
	int err = 0;
	u8 key1[bs];

	if ((err = crypto_cipher_setkey(ctx->child, ctx->key, ctx->keylen)))
	    return err;

	crypto_cipher_encrypt_one(ctx->child, key1, ctx->consts);

	return crypto_cipher_setkey(ctx->child, key1, bs);
}

static int crypto_xcbc_digest_setkey(struct crypto_shash *parent,
				     const u8 *inkey, unsigned int keylen)
{
	struct crypto_xcbc_ctx *ctx = crypto_shash_ctx(parent);

	if (keylen != crypto_cipher_blocksize(ctx->child))
		return -EINVAL;

	ctx->keylen = keylen;
	memcpy(ctx->key, inkey, keylen);
	ctx->consts = (u8*)ks;

	return _crypto_xcbc_digest_setkey(parent, ctx);
}

static int crypto_xcbc_digest_init(struct shash_desc *pdesc)
{
	struct crypto_xcbc_ctx *ctx = crypto_shash_ctx(pdesc->tfm);
	int bs = crypto_shash_blocksize(pdesc->tfm);

	ctx->len = 0;
	memset(ctx->odds, 0, bs);
	memset(ctx->prev, 0, bs);

	return 0;
}

static int crypto_xcbc_digest_update(struct shash_desc *pdesc, const u8 *p,
				     unsigned int len)
{
	struct crypto_shash *parent = pdesc->tfm;
	struct crypto_xcbc_ctx *ctx = crypto_shash_ctx(parent);
	struct crypto_cipher *tfm = ctx->child;
	int bs = crypto_shash_blocksize(parent);

	/* checking the data can fill the block */
	if ((ctx->len + len) <= bs) {
		memcpy(ctx->odds + ctx->len, p, len);
		ctx->len += len;
		return 0;
	}

	/* filling odds with new data and encrypting it */
	memcpy(ctx->odds + ctx->len, p, bs - ctx->len);
	len -= bs - ctx->len;
	p += bs - ctx->len;

	ctx->xor(ctx->prev, ctx->odds, bs);
	crypto_cipher_encrypt_one(tfm, ctx->prev, ctx->prev);

	/* clearing the length */
	ctx->len = 0;

	/* encrypting the rest of data */
	while (len > bs) {
		ctx->xor(ctx->prev, p, bs);
		crypto_cipher_encrypt_one(tfm, ctx->prev, ctx->prev);
		p += bs;
		len -= bs;
	}

	/* keeping the surplus of blocksize */
	if (len) {
		memcpy(ctx->odds, p, len);
		ctx->len = len;
	}

	return 0;
}

static int crypto_xcbc_digest_final(struct shash_desc *pdesc, u8 *out)
{
	struct crypto_shash *parent = pdesc->tfm;
	struct crypto_xcbc_ctx *ctx = crypto_shash_ctx(parent);
	struct crypto_cipher *tfm = ctx->child;
	int bs = crypto_shash_blocksize(parent);
	int err = 0;

	if (ctx->len == bs) {
		u8 key2[bs];

		if ((err = crypto_cipher_setkey(tfm, ctx->key, ctx->keylen)) != 0)
			return err;

		crypto_cipher_encrypt_one(tfm, key2,
					  (u8 *)(ctx->consts + bs));

		ctx->xor(ctx->prev, ctx->odds, bs);
		ctx->xor(ctx->prev, key2, bs);
		_crypto_xcbc_digest_setkey(parent, ctx);

		crypto_cipher_encrypt_one(tfm, out, ctx->prev);
	} else {
		u8 key3[bs];
		unsigned int rlen;
		u8 *p = ctx->odds + ctx->len;
		*p = 0x80;
		p++;

		rlen = bs - ctx->len -1;
		if (rlen)
			memset(p, 0, rlen);

		if ((err = crypto_cipher_setkey(tfm, ctx->key, ctx->keylen)) != 0)
			return err;

		crypto_cipher_encrypt_one(tfm, key3,
					  (u8 *)(ctx->consts + bs * 2));

		ctx->xor(ctx->prev, ctx->odds, bs);
		ctx->xor(ctx->prev, key3, bs);

		_crypto_xcbc_digest_setkey(parent, ctx);

		crypto_cipher_encrypt_one(tfm, out, ctx->prev);
	}

	return 0;
}

static int xcbc_init_tfm(struct crypto_tfm *tfm)
{
	struct crypto_cipher *cipher;
	struct crypto_instance *inst = (void *)tfm->__crt_alg;
	struct crypto_spawn *spawn = crypto_instance_ctx(inst);
	struct crypto_xcbc_ctx *ctx = crypto_tfm_ctx(tfm);
	int bs = crypto_tfm_alg_blocksize(tfm);

	cipher = crypto_spawn_cipher(spawn);
	if (IS_ERR(cipher))
		return PTR_ERR(cipher);

	switch(bs) {
	case 16:
		ctx->xor = xor_128;
		break;
	default:
		return -EINVAL;
	}

	ctx->child = cipher;
	ctx->odds = (u8*)(ctx+1);
	ctx->prev = ctx->odds + bs;
	ctx->key = ctx->prev + bs;

	return 0;
};

static void xcbc_exit_tfm(struct crypto_tfm *tfm)
{
	struct crypto_xcbc_ctx *ctx = crypto_tfm_ctx(tfm);
	crypto_free_cipher(ctx->child);
}

static int xcbc_create(struct crypto_template *tmpl, struct rtattr **tb)
{
	struct shash_instance *inst;
	struct crypto_alg *alg;
	int err;

	err = crypto_check_attr_type(tb, CRYPTO_ALG_TYPE_SHASH);
	if (err)
		return err;

	alg = crypto_get_attr_alg(tb, CRYPTO_ALG_TYPE_CIPHER,
				  CRYPTO_ALG_TYPE_MASK);
	if (IS_ERR(alg))
		return PTR_ERR(alg);

	switch(alg->cra_blocksize) {
	case 16:
		break;
	default:
		goto out_put_alg;
	}

	inst = shash_alloc_instance("xcbc", alg);
	err = PTR_ERR(inst);
	if (IS_ERR(inst))
		goto out_put_alg;

	err = crypto_init_spawn(shash_instance_ctx(inst), alg,
				shash_crypto_instance(inst),
				CRYPTO_ALG_TYPE_MASK);
	if (err)
		goto out_free_inst;

	inst->alg.base.cra_priority = alg->cra_priority;
	inst->alg.base.cra_blocksize = alg->cra_blocksize;
	inst->alg.base.cra_alignmask = alg->cra_alignmask;

	inst->alg.digestsize = alg->cra_blocksize;
	inst->alg.base.cra_ctxsize = sizeof(struct crypto_xcbc_ctx) +
				     ALIGN(alg->cra_blocksize * 3,
					   sizeof(void *));
	inst->alg.base.cra_init = xcbc_init_tfm;
	inst->alg.base.cra_exit = xcbc_exit_tfm;

	inst->alg.init = crypto_xcbc_digest_init;
	inst->alg.update = crypto_xcbc_digest_update;
	inst->alg.final = crypto_xcbc_digest_final;
	inst->alg.setkey = crypto_xcbc_digest_setkey;

	err = shash_register_instance(tmpl, inst);
	if (err) {
out_free_inst:
		shash_free_instance(shash_crypto_instance(inst));
	}

out_put_alg:
	crypto_mod_put(alg);
	return err;
}

static struct crypto_template crypto_xcbc_tmpl = {
	.name = "xcbc",
	.create = xcbc_create,
	.free = shash_free_instance,
	.module = THIS_MODULE,
};

static int __init crypto_xcbc_module_init(void)
{
	return crypto_register_template(&crypto_xcbc_tmpl);
}

static void __exit crypto_xcbc_module_exit(void)
{
	crypto_unregister_template(&crypto_xcbc_tmpl);
}

module_init(crypto_xcbc_module_init);
module_exit(crypto_xcbc_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("XCBC keyed hash algorithm");
