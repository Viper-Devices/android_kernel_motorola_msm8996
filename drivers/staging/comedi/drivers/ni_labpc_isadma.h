/*
 * ni_labpc ISA DMA support.
*/

#ifndef _NI_LABPC_ISADMA_H
#define _NI_LABPC_ISADMA_H

#define NI_LABPC_HAVE_ISA_DMA	IS_ENABLED(CONFIG_COMEDI_NI_LABPC_ISADMA)

#if NI_LABPC_HAVE_ISA_DMA

static inline bool labpc_have_dma_chan(struct comedi_device *dev)
{
	struct labpc_private *devpriv = dev->private;

	return (bool)devpriv->dma_chan;
}

int labpc_init_dma_chan(struct comedi_device *dev, unsigned int dma_chan);
void labpc_free_dma_chan(struct comedi_device *dev);
void labpc_setup_dma(struct comedi_device *dev, struct comedi_subdevice *s);

#else

static inline bool labpc_have_dma_chan(struct comedi_device *dev)
{
	return false;
}

static inline int labpc_init_dma_chan(struct comedi_device *dev,
				      unsigned int dma_chan)
{
	return -ENOTSUPP;
}

static inline void labpc_free_dma_chan(struct comedi_device *dev)
{
}

static inline void labpc_setup_dma(struct comedi_device *dev,
				   struct comedi_subdevice *s)
{
}

#endif

#endif /* _NI_LABPC_ISADMA_H */
