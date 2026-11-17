#include "MJPGRTPPayloadProcessor.h"

#include <platforms.h>
#include <BitFunctions.h>
#include <timing.h>

#include <IImage.h>
#include <CImage.h>
#include <irrArray.h>

#include <platforms.h>

//TODO use libjpeg-turbo on android and ios!!!
#if defined(ANDROID_PLATFORM) || defined(IOS_PLATFORM)
#define USE_SLOW_LIBJPEG
#endif
//#define USE_SLOW_LIBJPEG true

//ECF_A8R8G8B8 is better because it does not need conversion before rendering
#ifdef USE_SLOW_LIBJPEG
#define PIXEL_FORMAT ECF_R8G8B8
#else
#define PIXEL_FORMAT ECF_A8R8G8B8
#define TJPG_PIXEL_FORMAT TJPF_BGRA
#endif

#ifdef USE_SLOW_LIBJPEG
//TODO use libjpeg-turbo on android and ios!!!
#include <jpeglib/jpeglib.h> // use irrlicht jpeglib
#else
#include <turbojpeg.h>
#endif

#include <setjmp.h>
#include <fstream>
#include <iostream>

using namespace irr;
using namespace core;
using namespace video;

//Code from RFC2435 -----

using u_char = uint8_t;
using u_short = uint16_t;

/*
 * Table K.1 from JPEG spec.
 */
static const int jpeg_luma_quantizer[64] = {
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68, 109, 103, 77,
        24, 35, 55, 64, 81, 104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103, 99
};

/*
 * Table K.2 from JPEG spec.
 */
static const int jpeg_chroma_quantizer[64] = {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
};

/*
 * Call MakeTables with the Q factor and two u_char[64] return arrays
 */
void MakeTables(int q, u_char *lqt, u_char *cqt)
{
  int i;
  int factor = q;

  if (q < 1) factor = 1;
  if (q > 99) factor = 99;
  if (q < 50)
    q = 5000 / factor;
  else
    q = 200 - factor*2;
  for (i=0; i < 64; i++) {
    int lq = (jpeg_luma_quantizer[i] * q + 50) / 100;
    int cq = (jpeg_chroma_quantizer[i] * q + 50) / 100;

    /* Limit the quantizers to 1 <= q <= 255 */
    if (lq < 1) lq = 1;
    else if (lq > 255) lq = 255;
    lqt[i] = lq;

    if (cq < 1) cq = 1;
    else if (cq > 255) cq = 255;
    cqt[i] = cq;
  }
}

static const u_char lum_dc_codelens[] = {
        0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static const u_char lum_dc_symbols[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static const u_char lum_ac_codelens[] = {
        0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

u_char lum_ac_symbols[] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa,
};

static const u_char chm_dc_codelens[] = {
        0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

static const u_char chm_dc_symbols[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

u_char chm_ac_codelens[] = {
        0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

static const u_char chm_ac_symbols[] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa,
};

static u_char * MakeQuantHeader(u_char *p, const u_char *qt, int tableNo)
{
        *p++ = 0xff;
        *p++ = 0xdb;            /* DQT */
        *p++ = 0;               /* length msb */
        *p++ = 67;              /* length lsb */
        *p++ = tableNo;
        memcpy(p, qt, 64);
        return (p + 64);
}

static u_char *
MakeHuffmanHeader(u_char *p, const u_char *codelens, int ncodes,
                  const u_char *symbols, int nsymbols, int tableNo,
                  int tableClass)
{
        *p++ = 0xff;
        *p++ = 0xc4;            /* DHT */
        *p++ = 0;               /* length msb */
        *p++ = 3 + ncodes + nsymbols; /* length lsb */
        *p++ = (tableClass << 4) | tableNo;
        memcpy(p, codelens, ncodes);
        p += ncodes;
        memcpy(p, symbols, nsymbols);
        p += nsymbols;
        return (p);
}

static u_char *
MakeDRIHeader(u_char *p, u_short dri) {
        *p++ = 0xff;
        *p++ = 0xdd;            /* DRI */
        *p++ = 0x0;             /* length msb */
        *p++ = 4;               /* length lsb */
        *p++ = dri >> 8;        /* dri msb */
        *p++ = dri & 0xff;      /* dri lsb */
        return (p);
}

/*
 *  Arguments:
 *    type, width, height: as supplied in RTP/JPEG header
 *    lqt, cqt: quantization tables as either derived from
 *         the Q field using MakeTables() or as specified
 *         in section 4.2.
 *    dri: restart interval in MCUs, or 0 if no restarts.
 *
 *    p: pointer to return area
 *
 *  Return value:
 *    The length of the generated headers.
 *
 *    Generate a frame and scan headers that can be prepended to the
 *    RTP/JPEG data payload to produce a JPEG compressed image in
 *    interchange format (except for possible trailing garbage and
 *    absence of an EOI marker to terminate the scan).
 */
static int MakeHeaders(u_char *p, int type, int w, int h, const u_char *lqt,
                const u_char *cqt, u_short dri)
{
        u_char *start = p;

        /* convert from blocks to pixels */
        w <<= 3;
        h <<= 3;
        *p++ = 0xff;
        *p++ = 0xd8;            /* SOI */

        p = MakeQuantHeader(p, lqt, 0);
        p = MakeQuantHeader(p, cqt, 1);

        if (dri != 0)
                p = MakeDRIHeader(p, dri);

        *p++ = 0xff;
        *p++ = 0xc0;            /* SOF */
        *p++ = 0;               /* length msb */
        *p++ = 17;              /* length lsb */
        *p++ = 8;               /* 8-bit precision */
        *p++ = h >> 8;          /* height msb */
        *p++ = h;               /* height lsb */
        *p++ = w >> 8;          /* width msb */
        *p++ = w;               /* wudth lsb */
        *p++ = 3;               /* number of components */
        *p++ = 0;               /* comp 0 */
        if (type == 0)
                *p++ = 0x21;    /* hsamp = 2, vsamp = 1 */
        else
                *p++ = 0x22;    /* hsamp = 2, vsamp = 2 */
        *p++ = 0;               /* quant table 0 */
        *p++ = 1;               /* comp 1 */
        *p++ = 0x11;            /* hsamp = 1, vsamp = 1 */
        *p++ = 1;               /* quant table 1 */
        *p++ = 2;               /* comp 2 */
        *p++ = 0x11;            /* hsamp = 1, vsamp = 1 */
        *p++ = 1;               /* quant table 1 */
        p = MakeHuffmanHeader(p, lum_dc_codelens,
                              sizeof(lum_dc_codelens),
                              lum_dc_symbols,
                              sizeof(lum_dc_symbols), 0, 0);
        p = MakeHuffmanHeader(p, lum_ac_codelens,
                              sizeof(lum_ac_codelens),
                              lum_ac_symbols,
                              sizeof(lum_ac_symbols), 0, 1);
        p = MakeHuffmanHeader(p, chm_dc_codelens,
                              sizeof(chm_dc_codelens),
                              chm_dc_symbols,
                              sizeof(chm_dc_symbols), 1, 0);
        p = MakeHuffmanHeader(p, chm_ac_codelens,
                              sizeof(chm_ac_codelens),
                              chm_ac_symbols,
                              sizeof(chm_ac_symbols), 1, 1);
        *p++ = 0xff;
        *p++ = 0xda;            /* SOS */
        *p++ = 0;               /* length msb */
        *p++ = 12;              /* length lsb */
        *p++ = 3;               /* 3 components */
        *p++ = 0;               /* comp 0 */
        *p++ = 0;               /* huffman table 0 */
        *p++ = 1;               /* comp 1 */
        *p++ = 0x11;            /* huffman table 1 */
        *p++ = 2;               /* comp 2 */
        *p++ = 0x11;            /* huffman table 1 */
        *p++ = 0;               /* first DCT coeff */
        *p++ = 63;              /* last DCT coeff */
        *p++ = 0;               /* sucessive approx. */

        return (p - start);
};

//Code from RFC2435 end -----

class MJPGRTPPayloadProcessorPrivate{

	public:
	
	static constexpr uint32_t maxHeaderSize = 2*935;//935 theoretical maximum, double to be safe
	uint8_t header[maxHeaderSize];
	int headerSize;
	
	static constexpr uint32_t bufSize = maxHeaderSize+2*1920*1080*4;//double HD uncompressed as upper boundary
	uint8_t target[bufSize];
	uint32_t targetOffset;
	
	uint8_t* finalImage;
	uint32_t finalImageSize;
	
	//decoding helpers
	array<u8> output;
	array<u8*> rowPtr;
	
	uint16_t lastSqNr;
	bool isAssembling;
	
	//mjpg header
	uint8_t typeSpecific;
	uint32_t fragmentOffset;
	uint8_t type;
	uint8_t Q;
	uint32_t width, height;//unit: 8 pixels if mode==0, pixels if mode==1
	
	//restarts (unused if restartInterval = 0)
	uint16_t restartInterval, restartCount;
	bool F, L;
	
	//Quantization tables if Q>=128
	uint8_t MBZ;
	uint8_t precision;
	uint16_t length;
	std::vector<uint8_t> qData;
	
	uint8_t mode;//0: standard mjpg, 1: custom mjpg (see RTPSender)
	
	MJPGRTPPayloadProcessorPrivate(){
		mode = 0;
		reset();
	}
	
	void reset(){
		isAssembling = false;
		restartInterval = 0;
		restartCount = 0;
		headerSize = 0;
		targetOffset = maxHeaderSize;
	}
	
	void makeHeader(){
		if(qData.size()<128){
			qData.resize(128);
		}
		u_char* lqt = &(qData[0]); u_char* cqt = &(qData[64]);
		if(!(Q>=128 && length==128)){//quantization comes with first paket
			MakeTables(Q, lqt, cqt);//TODO optimize by caching with Q value as key if Q<128
		}
		headerSize = MakeHeaders(header, type, width, height, lqt, cqt, restartInterval);
		finalImage = &target[maxHeaderSize-headerSize];
		memcpy(finalImage, header, headerSize);
	}
	
	bool update(FrameBasedRTPReceiver::Buffer* b){
		if(b && b->used>8){
			//std::cout << "b->sequenceNumber: " << b->sequenceNumber << std::endl;
			uint8_t* data = (uint8_t*)&(b->data[b->start]);
			uint32_t used = b->used;
			if(isAssembling){
				if(b->sequenceNumber==lastSqNr+1){
					uint32_t offset = b->payloadType==77?0:((type>=64 && type<=127)?12:8);
					uint32_t toCopy = used-offset;
					if(targetOffset+toCopy<=bufSize){
						memcpy(&(target[targetOffset]), &(data[offset]), toCopy);
						targetOffset += toCopy;
					}
					lastSqNr = b->sequenceNumber;
					if(b->marker){
						if(b->payloadType==77){//custom mjpg payload see RTPSender
							mode = 1;
							finalImage = target;
						}else{
							mode = 0;
							makeHeader();
							target[targetOffset] = 0xFF; targetOffset++;
							target[targetOffset] = 0xD9; targetOffset++;
						}
						//std::cout << "targetOffset: " << targetOffset << std::endl;
						finalImageSize = &(target[targetOffset]) - finalImage;//pointer arithmetic
						isAssembling = false;
					}
					return b->marker;//last packet is marked
				}else{
					std::cout << getSecs() << "s MJPG Error: old sequence number: " << lastSqNr << " new: " << b->sequenceNumber << std::endl;
					isAssembling = false;
					//return false;
				}
			}
			if(!isAssembling){
				lastSqNr = b->sequenceNumber;
				if(b->payloadType==77){//custom mjpg payload see RTPSender
					mode = 1;
					if(data[0]==0xFF && data[1]==0xFF){//magic number does not occur in jpeg and marks first frame since payload begins with this
						uint32_t offset = 2;
						width = readBigEndian<uint16_t>(data, offset);
						height = readBigEndian<uint16_t>(data, offset);
//						std::cout << "width: " << width << " height: " << height << std::endl;
						isAssembling = !b->marker;
						targetOffset = used-offset;//amount
						memcpy(target, &(data[offset]), targetOffset);
						return b->marker;
					}
				}else{
					mode = 0;
					reset();
					typeSpecific = data[0];
					fragmentOffset = static_cast<uint32_t>(data[3]) |  (static_cast<uint32_t>(data[2]) << 8) | (static_cast<uint32_t>(data[1]) << 16);
					if(fragmentOffset==0){//must be the first packet of the frame to get the header information
						type = data[4];
						Q = data[5];
						width = static_cast<uint32_t>(data[6]);// * 8;
						height = static_cast<uint32_t>(data[7]);// * 8;
						uint32_t offset = 8;
						//Resets
						if(type>=64 && type<=127){
							if(used>12){
								restartInterval = (static_cast<uint16_t>(data[8]) << 8) | static_cast<uint16_t>(data[9]);
								restartCount = ((static_cast<uint16_t>(data[10]) << 8) | static_cast<uint16_t>(data[11])) & 0b0011111111111111;
								F = getBit(data[10], 0);
								L = getBit(data[10], 1);
								offset += 4;
							}else{
								std::cout << "MJPG: Error in reset header." << std::endl;
								isAssembling = false;
								return false;
							}
						}
						//Quantization Tables
						if(Q>=128){
							MBZ = data[offset];
							precision = data[offset+1];
							length = (static_cast<uint16_t>(data[offset+2]) << 8) | static_cast<uint16_t>(data[offset+3]);
							offset += 4;
							if(offset+length<=used){
								qData.resize(length);
								memcpy(qData.data(), &(data[offset]), length);
								offset += length;
							}else{
								isAssembling = false;
								std::cout << "MJPG: Error in quantization table." << std::endl;
								return false;
							}
						}
						uint32_t toCopy = used-offset;
						if(targetOffset+toCopy<=bufSize){
							memcpy(&(target[targetOffset]), &(data[offset]), toCopy);
							targetOffset += toCopy;
						}
						isAssembling = true;
					}else{
						//std::cout << "MJPG: Not the first packet." << std::endl;
						isAssembling = false;
						return false;
					}
				}
			}
		}
		return false;
	}
	
	#ifdef USE_SLOW_LIBJPEG
	// struct for handling jpeg errors
    struct irr_jpeg_error_mgr
    {
        // public jpeg error fields
        struct jpeg_error_mgr pub;

        // for longjmp, to return to caller on a fatal error
        jmp_buf setjmp_buffer;
    };

	static void init_source (j_decompress_ptr cinfo)
	{
		// DO NOTHING
	}

	static boolean fill_input_buffer (j_decompress_ptr cinfo)
	{
		// DO NOTHING
		return TRUE;
	}

	static void skip_input_data (j_decompress_ptr cinfo, long count)
	{
		jpeg_source_mgr * src = cinfo->src;
		if(count > 0)
		{
			if((size_t)count>src->bytes_in_buffer){count = src->bytes_in_buffer;}//changed
			src->bytes_in_buffer -= count;
			src->next_input_byte += count;
		}
	}

	static void term_source (j_decompress_ptr cinfo)
	{
		// DO NOTHING
	}

	static void error_exit (j_common_ptr cinfo)
	{
		// unfortunately we need to use a goto rather than throwing an exception
		// as gcc crashes under linux crashes when using throw from within
		// extern c code

		// Always display the message
		(*cinfo->err->output_message) (cinfo);

		// cinfo->err really points to a irr_error_mgr struct
		irr_jpeg_error_mgr *myerr = (irr_jpeg_error_mgr*) cinfo->err;
		longjmp(myerr->setjmp_buffer, 1);
	}


	static void output_message(j_common_ptr cinfo)
	{
		//changed: 
		// display the error message.
		c8 temp1[JMSG_LENGTH_MAX];
		(*cinfo->err->format_message)(cinfo, temp1);
		//removed to make it thread safe: os::Printer::log
		std::cout << "JPEG FATAL ERROR: " << temp1 << std::endl;
		//jump also here in case of error output without exit to avoid broken images
		irr_jpeg_error_mgr *myerr = (irr_jpeg_error_mgr*) cinfo->err;
		longjmp(myerr->setjmp_buffer, 1);
		//changed end
	}

	bool fillImage(irr::video::IImage* img){//TODO optimize
		if((mode==0 && headerSize<=0) || targetOffset<=0){return false;}
		// allocate and initialize JPEG decompression object
		struct jpeg_decompress_struct cinfo;
		struct irr_jpeg_error_mgr jerr;

		//We have to set up the error handler first, in case the initialization
		//step fails.  (Unlikely, but it could happen if you are out of memory.)
		//This routine fills in the contents of struct jerr, and returns jerr's
		//address which we place into the link field in cinfo.

		cinfo.err = jpeg_std_error(&jerr.pub);
		cinfo.err->error_exit = error_exit;
		cinfo.err->output_message = output_message;

		// compatibility fudge:
		// we need to use setjmp/longjmp for error handling as gcc-linux
		// crashes when throwing within external c code
		if (setjmp(jerr.setjmp_buffer))
		{
			// If we get here, the JPEG code has signaled an error.
			// We need to clean up the JPEG object and return.

			jpeg_destroy_decompress(&cinfo);

			return false;
		}

		// Now we can initialize the JPEG decompression object.
		jpeg_create_decompress(&cinfo);//TODO optimize not recreate all time

		// specify data source
		jpeg_source_mgr jsrc;

		// Set up data pointer
		std::cout << "finalImageSize: " << finalImageSize << std::endl;
		jsrc.bytes_in_buffer = finalImageSize;
		jsrc.next_input_byte = (JOCTET*)finalImage;
		cinfo.src = &jsrc;

		jsrc.init_source = init_source;
		jsrc.fill_input_buffer = fill_input_buffer;
		jsrc.skip_input_data = skip_input_data;
		jsrc.resync_to_restart = jpeg_resync_to_restart;
		jsrc.term_source = term_source;

		// Decodes JPG input from whatever source
		// Does everything AFTER jpeg_create_decompress
		// and BEFORE jpeg_destroy_decompress
		// Caller is responsible for arranging these + setting up cinfo

		// read file parameters with jpeg_read_header()
		jpeg_read_header(&cinfo, TRUE);

		bool useCMYK=false;
		if (cinfo.jpeg_color_space==JCS_CMYK)
		{
			cinfo.out_color_space=JCS_CMYK;
			cinfo.out_color_components=4;
			useCMYK=true;
		}
		else
		{
			cinfo.out_color_space=JCS_RGB;
			cinfo.out_color_components=3;
		}
		cinfo.output_gamma=2.2;
		cinfo.do_fancy_upsampling=FALSE;

		// Start decompressor
		jpeg_start_decompress(&cinfo);

		// Get image data
		u16 rowspan = cinfo.image_width * cinfo.out_color_components;
		u32 width = cinfo.image_width;
		u32 height = cinfo.image_height;
		
		u8* output;
		if(useCMYK){
			// Allocate memory for buffer
			this->output.set_used(rowspan * height);//u8* output = new u8[rowspan * height];
			output = &(this->output[0]);
		}else{
			output = (u8*)(img->getData());//size matches: width * 3 * height
		}

		// Here we use the library's state variable cinfo.output_scanline as the
		// loop counter, so that we don't have to keep track ourselves.
		// Create array of row pointers for lib
		rowPtr.set_used(height);//rowPtr = new u8* [height];

		for( u32 i = 0; i < height; i++ )
			rowPtr[i] = &output[ i * rowspan ];

		u32 rowsRead = 0;

		while( cinfo.output_scanline < cinfo.output_height )
			rowsRead += jpeg_read_scanlines( &cinfo, &rowPtr[rowsRead], cinfo.output_height - rowsRead );

		// Finish decompression

		jpeg_finish_decompress(&cinfo);

		// Release JPEG decompression object
		// This is an important step since it will release a good deal of memory.
		jpeg_destroy_decompress(&cinfo);

		// convert image
		if (useCMYK){
			const u32 size = 3*width*height;
			u8* data = (u8*)img->getData();
			if (data)
			{
				for (u32 i=0,j=0; i<size; i+=3, j+=4)
				{
					f32 oj3 = output[j+3]/255.f;
					data[i+0] = (u8)(output[j+2]*oj3);
					data[i+1] = (u8)(output[j+1]*oj3);
					data[i+2] = (u8)(output[j+0]*oj3);
				}
			}
		}
		//else already converted since target image buffer is used

		return true;

	}
	#else
	bool fillImage(irr::video::IImage* img){
		dimension2d<irr::u32> size = getSize();
		tjhandle tjInstance = tjInitDecompress();//TODO more efficient?
		bool success = tjDecompress2(tjInstance, finalImage, finalImageSize, (unsigned char*)(img->getData()), size.Width, 0, size.Height, TJPG_PIXEL_FORMAT, 0) == 0;//flags 0
		if(!success){
			std::cerr << tjGetErrorStr() << std::endl;
//			writeJPEG("/dev/shm/error.jpg");
//			exit(0);
		}
		tjDestroy(tjInstance);
		return true;//success;
	}
	#endif
	
	irr::core::dimension2d<irr::u32> getSize() const{
		return mode==0?dimension2d<u32>(width*8, height*8):dimension2d<u32>(width, height);
	}
	
	void writeJPEG(const std::string& path){
		if((mode==1 || headerSize>0) && targetOffset>0){
			std::ofstream fp(path.c_str(), std::ofstream::binary | std::ofstream::trunc);
			if(fp.good()){
				fp.write((const char*)finalImage, finalImageSize);
			}else{
				std::cerr << "Error writing to: " << path << std::endl;
			}
		}
	}
	
};
	
MJPGRTPPayloadProcessor::MJPGRTPPayloadProcessor(){
	p = new MJPGRTPPayloadProcessorPrivate();
}

MJPGRTPPayloadProcessor::~MJPGRTPPayloadProcessor(){
	delete p;
}
	
bool MJPGRTPPayloadProcessor::update(FrameBasedRTPReceiver::Buffer* b){
	return p->update(b);
//	if(p->update(b)){
//		writeJPEG("/dev/shm/test.jpg");
//		exit(-1);
//	}
//	return false;
}

irr::video::IImage* MJPGRTPPayloadProcessor::createImageForSize() const{
	return new CImage(PIXEL_FORMAT, p->getSize());
}

void MJPGRTPPayloadProcessor::writeJPEG(const std::string& path){
	p->writeJPEG(path);
}

irr::core::dimension2d<irr::u32> MJPGRTPPayloadProcessor::getSize() const{
	return p->getSize();
}
	
bool MJPGRTPPayloadProcessor::fillImage(irr::video::IImage* img){
	return p->fillImage(img);
}
