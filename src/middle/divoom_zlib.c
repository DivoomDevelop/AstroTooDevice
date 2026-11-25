#include "divoom.h"
//#include "zlib.h"

//实现解压和压缩
//大概流程:
//1,初始化句柄inflateInit,参数z_streamp-->zmalloc, zfree, next_in = NULL, avail_in = 0, opaque = NULL(?)
//2,avail_in是读入的字节数量,需要外部的一个buf,大小自己定义,最好是做成可重入的那种?用栈空间?如果出错可以用inflateEnd,参数z_streamp来表达错误
//3,next_in给为读入的buf(2提到的)
//4,开始循环解压,根据avail_out是否为0来判断是否当前的next_in的buf是否全部解压完毕,avail_out是当前buf的剩余空间,如果剩余空间为0,那么证明还需要再进行解压缩
//	或者是判断avail_in是否=0,等于0说明这一次要解压缩的数据没了
//5,解压先给定相应的buf大小和size,avail_out最初大小,next_out给定输出buf,使用inflate来进行解压,一般来说第二个参数只需要Z_NO_FLUSH,或者MZ_FINISH,其他的值参考zlib文档,没看
//6,inflate的返回值:Z_STREAM_ERROR要assert,失败,严重;Z_NEED_DICT错误,return;Z_DATA_ERROR和Z_MEM_ERROR错误,return,并且需要inflateEnd;Z_STREAM_END为当前输入buf解压结束
//7,需要继续读取的标志应该是返回Z_OK并且avail_out是0,并且腾出当前缓冲区,可以按照1:10的比例来申请空间

//刚发现zlib有提供compress和unconpress函数,这里改了

#define		TAG										"ZLIB"
#define		DIVOOM_ZLIB_PRINT                       divoom_print


void *divoom_zlib_malloc(void *value, uint32 item, uint32 size)
{
	(void)value;
	return divoom_malloc(item*size, DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);
}

void divoom_zlib_free(void *value, void *ptr)
{
	(void)value;
	divoom_free(ptr);
}


#if 0
#define		IN_BUF_SIZE								(1<<10)
#define		OUT_BUF_SIZE							(2<<10)

typedef struct {
	uint8			*in_buf;
	uint8			*out_buf;
	void			*safe_lock;
	z_stream		zlib_handle;
#ifdef DIVOOM_ZLIB_TEST
	z_stream		zlib_handle_test;
#endif
}divoom_zlib_struct, *Pdivoom_zlib_struct;

Pdivoom_zlib_struct pdivoom_zlib_handle = NULL;

//初始化zlib句柄和相关缓冲区的申请
void divoom_zlib_init(void)
{
	if(pdivoom_zlib_handle != NULL)
	{
		DIVOOM_ZLIB_PRINT("%s--%d, Had Init!\n", __func__, __LINE__);
		return;
	}
	pdivoom_zlib_handle = divoom_malloc(sizeof(divoom_zlib_struct), DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);
	memset(pdivoom_zlib_handle, 0, sizeof(divoom_zlib_struct));

	pdivoom_zlib_handle->in_buf = divoom_malloc(sizeof(uint8)*IN_BUF_SIZE, DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);
	memset(pdivoom_zlib_handle->in_buf, 0, sizeof(uint8)*IN_BUF_SIZE);
	pdivoom_zlib_handle->out_buf = divoom_malloc(sizeof(uint8)*OUT_BUF_SIZE, DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);
	memset(pdivoom_zlib_handle->out_buf, 0, sizeof(uint8)*OUT_BUF_SIZE);

	pdivoom_zlib_handle->safe_lock = divoom_system_create_lock();

	return;
Wrong:
	divoom_free((pdivoom_zlib_handle->in_buf == NULL)?NULL:pdivoom_zlib_handle->in_buf);
	divoom_free((pdivoom_zlib_handle->out_buf == NULL)?NULL:pdivoom_zlib_handle->out_buf);
	divoom_free((pdivoom_zlib_handle == NULL)?NULL:pdivoom_zlib_handle);
	return;
}

uint8 *divoom_zlib_compress(uint8 *zlib_data, uint32 data_len)
{
	return NULL;
}

//通过几张图片和当前显示屏的长宽高可以得到原始数据和aes加密后的数据的总大小
//aes加密不改变数据的长度
//返回数据和数据的长度
uint8 *divoom_zlib_decompress_pic(uint8 *zlib_data, uint32 data_len, uint8 pic_num, uint8 height, uint8 wight, uint8 device_num, uint32 *return_data_len)
{
	uint32 read_offset = 0;
	uint32 cur_read = 0;
	uint32 decompress_len = 0;
	uint32 decompress_offset = 0;
	int ret = Z_OK;
	uint32 all_len = ((pic_num<=0)?1:pic_num)*((height<=0)?SCREEN_HEIGHT:height)*((wight<=0)?SCREEN_WIDTH:wight)*BYTES_PRE_DOT*device_num;

	DIVOOM_ZLIB_PRINT("%s--%d-----all_len = %d, pic_num = %d, device_num = %d\n", __func__, __LINE__, all_len, pic_num, device_num);
	if(zlib_data == NULL || data_len == 0 || pdivoom_zlib_handle == NULL)
	{
		DIVOOM_ZLIB_PRINT("%s--%d---SomeWrong in head!\n", __func__, __LINE__);
		return NULL;
	}

	divoom_system_get_lock(pdivoom_zlib_handle->safe_lock);

	pdivoom_zlib_handle->zlib_handle.zalloc = divoom_zlib_malloc;
	pdivoom_zlib_handle->zlib_handle.zfree = divoom_zlib_free;
	pdivoom_zlib_handle->zlib_handle.opaque = NULL;

	pdivoom_zlib_handle->zlib_handle.avail_in = 0;
	pdivoom_zlib_handle->zlib_handle.next_in = pdivoom_zlib_handle->in_buf;

	if(inflateInit(&pdivoom_zlib_handle->zlib_handle)!= Z_OK)
	{
		DIVOOM_ZLIB_PRINT("%s---%d---->inflateInit fail!!!!!\n", __func__, __LINE__);
		divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
		return NULL;
	}

	uint8 *decompress_buf = divoom_malloc(sizeof(uint8)*all_len, DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);

	memset(decompress_buf, 0, sizeof(uint8)*all_len);
	if(return_data_len)
		*return_data_len = all_len;

	//开始解压缩
	do
	{
		cur_read = (data_len>IN_BUF_SIZE)?IN_BUF_SIZE:data_len;
		memcpy(pdivoom_zlib_handle->in_buf, (char *)(zlib_data+read_offset), cur_read);
		data_len -= cur_read;
		read_offset += cur_read;
		//DIVOOM_ZLIB_PRINT("LINE:%d---->cur_read = %d, data_len = %d, read_offset = %d\n", __LINE__, cur_read, data_len, read_offset);
		pdivoom_zlib_handle->zlib_handle.next_in = pdivoom_zlib_handle->in_buf;
		pdivoom_zlib_handle->zlib_handle.avail_in = cur_read;
		do
		{
			pdivoom_zlib_handle->zlib_handle.next_out = pdivoom_zlib_handle->out_buf;
			pdivoom_zlib_handle->zlib_handle.avail_out = OUT_BUF_SIZE;
			ret = inflate(&pdivoom_zlib_handle->zlib_handle, Z_NO_FLUSH);
			DIVOOM_ZLIB_PRINT("inflate return %d\n", ret);
			switch(ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
					break;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
				case Z_STREAM_ERROR:
					//need inflateEnd
					(void)inflateEnd(&pdivoom_zlib_handle->zlib_handle);
					DIVOOM_ZLIB_PRINT("inflate Wrong, Wrong id = %d\n", ret);
					if(decompress_buf != NULL)
						divoom_free(decompress_buf);
					divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
					return NULL;
			}
			decompress_len = OUT_BUF_SIZE - pdivoom_zlib_handle->zlib_handle.avail_out;
			//DIVOOM_ZLIB_PRINT("LINE:%d-->decompress_len = %d, pdivoom_zlib_handle->zlib_handle.avail_out = %d\n", __LINE__, decompress_len, pdivoom_zlib_handle->zlib_handle.avail_out);
			memcpy(decompress_buf+decompress_offset, pdivoom_zlib_handle->out_buf, decompress_len);
			decompress_offset += decompress_len;
			//DIVOOM_ZLIB_PRINT("decompress_offset = %d\n", decompress_offset);
		}while(pdivoom_zlib_handle->zlib_handle.avail_out == 0);
		//↑这里如果可用空间为0,那么需要再次
	}while(ret != Z_STREAM_END || data_len > 0);
	//↑读到结尾并且当前数据读完了就结束了

	(void)inflateEnd(&pdivoom_zlib_handle->zlib_handle);
	divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);

	return decompress_buf;
}

//压缩的函数一般用不到,拿来做测试用
#ifdef DIVOOM_ZLIB_TEST
//测试函数直接写到文件里面
void divoom_zlib_compress(uint8 *data, uint32 data_len, char *file_name)
{
	uint32 cur_read = 0, read_offset = 0;
	int ret;
	uint32 compress_len = 0;
	uint8  flash_flag = 0;
	if(pdivoom_zlib_handle == NULL)
	{
		return;
	}
	FILE *fd = divoom_port_file_open(file_name, "w");
	if(fd == NULL)
	{
		return;
	}
	divoom_system_get_lock(pdivoom_zlib_handle->safe_lock);
	pdivoom_zlib_handle->zlib_handle_test.zalloc = divoom_zlib_malloc;
	pdivoom_zlib_handle->zlib_handle_test.zfree = divoom_zlib_free;
	pdivoom_zlib_handle->zlib_handle_test.opaque = NULL;
	if((ret = deflateInit(&pdivoom_zlib_handle->zlib_handle_test, Z_DEFAULT_COMPRESSION)) != Z_OK)
	{
		divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
		return;
	}
	do
	{
		cur_read = (data_len > OUT_BUF_SIZE)?OUT_BUF_SIZE:data_len;
		pdivoom_zlib_handle->zlib_handle_test.avail_in = cur_read;
		memset(pdivoom_zlib_handle->out_buf, 0, sizeof(uint8)*OUT_BUF_SIZE);
		memcpy(pdivoom_zlib_handle->out_buf, data+read_offset, cur_read);
		data_len -= cur_read;
		read_offset += cur_read;
		//DIVOOM_ZLIB_PRINT("%d------>cur_read = %d, data_len = %d\n", __LINE__, cur_read, data_len);
		flash_flag = (data_len == 0)?Z_FINISH:Z_NO_FLUSH;
		pdivoom_zlib_handle->zlib_handle_test.next_in = pdivoom_zlib_handle->out_buf;

		do
		{
			pdivoom_zlib_handle->zlib_handle_test.avail_out = IN_BUF_SIZE;
			memset(pdivoom_zlib_handle->in_buf, 0, IN_BUF_SIZE);
			pdivoom_zlib_handle->zlib_handle_test.next_out = pdivoom_zlib_handle->in_buf;
			//要判断是不结束的还是要结束的?
			ret = deflate(&pdivoom_zlib_handle->zlib_handle_test, flash_flag);
			//DIVOOM_ZLIB_PRINT("deflate ret = %d\n", ret);
			if(ret == Z_STREAM_ERROR)
			{
				DIVOOM_ZLIB_PRINT("\n\nWrong In Here %s--%d\n\n\n", __func__, __LINE__);
				divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
				return;
			}
			compress_len = IN_BUF_SIZE - pdivoom_zlib_handle->zlib_handle_test.avail_out;
			//DIVOOM_ZLIB_PRINT("compress_len = %d, pdivoom_zlib_handle->zlib_handle_test.avail_out = %d\n", compress_len, pdivoom_zlib_handle->zlib_handle_test.avail_out);
			//这里要用传进去的buf,不能使用zlib_handle_test.next_out
			if((ret = divoom_port_file_write(pdivoom_zlib_handle->in_buf, 1, compress_len, fd)) != compress_len)
			{
				DIVOOM_ZLIB_PRINT("%s--%d:Write Fail!!!!!, Write len = %d, compress_len = %d\n", __func__, __LINE__, ret, compress_len);
				(void)deflateEnd(&pdivoom_zlib_handle->zlib_handle_test);
				divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
				return;
			}
		}while(pdivoom_zlib_handle->zlib_handle_test.avail_out == 0);
	}while(data_len > 0);
	(void)deflateEnd(&pdivoom_zlib_handle->zlib_handle_test);
	divoom_port_file_close(fd);
	divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
	return;
}

//给一个原始文件名字,打开,读出来数据
//先压缩,返回数据,保存为原始名字+compress,在解压这个文件,保存为原始名字+decompress
void divoom_zlib_test_compress_and_decompress(char *origin_name)
{
	uint32 origin_len = 0;
	uint32 compress_len = 0;
	uint8  *origin_data = NULL;
	uint8  *compress_buf = NULL;
	uint8  *decompress_buf = NULL;
	FILE *fd_origin = divoom_port_file_open(origin_name, "r");
	FILE *fd_compress = NULL;
	FILE *fd_decompress = NULL;

	if(fd_origin == NULL)
	{
		DIVOOM_ZLIB_PRINT("SomeWrong In Here %s---%d\n", __func__, __LINE__);
		return;
	}
	divoom_port_file_seek(fd_origin, 0, SEEK_SET);
	divoom_port_file_seek(fd_origin, 0, SEEK_END);
	origin_len = divoom_port_file_tell(fd_origin);
	divoom_port_file_seek(fd_origin, 0, SEEK_SET);

	//DIVOOM_ZLIB_PRINT("origin_len = %d\n", origin_len);
	origin_data = divoom_malloc(sizeof(uint8)*origin_len, DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);

	memset(origin_data, 0, sizeof(uint8)*origin_len);

	if((compress_len = divoom_port_file_read(origin_data, 1, origin_len, fd_origin)) != origin_len)
	{
		DIVOOM_ZLIB_PRINT("origin_len = %d, read len = %d\n", origin_len, compress_len);
		return;
	}
	//DIVOOM_ZLIB_PRINT("origin_len = %d\n", origin_len);
	divoom_port_file_close(fd_origin);

	divoom_zlib_compress(origin_data, origin_len, DIVOOM_LEGACY_SD_ROOT "/pic2-compress");
	divoom_free(origin_data);

	//fd_compress = divoom_port_file_open(DIVOOM_LEGACY_SD_ROOT "/pic2-compress", "r");
	fd_compress = divoom_port_file_open(DIVOOM_LEGACY_SD_ROOT "/pic2-compress", "r");
	if(fd_compress == NULL)
	{
		DIVOOM_ZLIB_PRINT("SomeWrong In Here %s---%d\n", __func__, __LINE__);
		return;
	}
	divoom_port_file_seek(fd_compress, 0, SEEK_SET);
	divoom_port_file_seek(fd_compress, 0, SEEK_END);
	compress_len = divoom_port_file_tell(fd_compress);
	divoom_port_file_seek(fd_compress, 0, SEEK_SET);
	compress_buf = divoom_malloc(sizeof(uint8)*compress_len, DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);

	memset(compress_buf, 0, sizeof(uint8)*compress_len);

	compress_len = divoom_port_file_read(compress_buf, 1, compress_len, fd_compress);
	//DIVOOM_ZLIB_PRINT("compress_len = %d\n", compress_len);
	divoom_port_file_close(fd_compress);

	decompress_buf = divoom_zlib_decompress_pic(compress_buf, compress_len, 1, SCREEN_HEIGHT, SCREEN_WIDTH, 1, NULL);
	divoom_free(compress_buf);

	if(decompress_buf == NULL)
	{
		DIVOOM_ZLIB_PRINT("divoom_zlib_decompress return NULL!!!!!\n");
		return;
	}

	fd_decompress = divoom_port_file_open(DIVOOM_LEGACY_SD_ROOT "/pic2-decompress", "wb");
	if(fd_decompress == NULL)
	{
		DIVOOM_ZLIB_PRINT("SomeWrong In Here %s---%d\n", __func__, __LINE__);
		return;
	}
	divoom_port_file_write(decompress_buf, 1, 1*3*SCREEN_HEIGHT*SCREEN_HEIGHT, fd_decompress);
	divoom_port_file_close(fd_decompress);
	divoom_free(decompress_buf);

}

#endif

#else

#if 0
typedef struct {
	void			*safe_lock;
	//两个函数就可以了
	alloc_func		zalloc;
	free_func		zfree;
}divoom_zlib_struct, *Pdivoom_zlib_struct;

Pdivoom_zlib_struct pdivoom_zlib_handle = NULL;

#endif
void divoom_zlib_init(void)
{
#if 0
	if(pdivoom_zlib_handle != NULL)
	{
		DIVOOM_ZLIB_PRINT("%s--%d, Had Init!\n", __func__, __LINE__);
		return;
	}
	pdivoom_zlib_handle = divoom_malloc(sizeof(divoom_zlib_struct), DIVOOM_MODEL_THIRD_LIB, __LINE__, __FILE__);

	memset(pdivoom_zlib_handle, 0, sizeof(divoom_zlib_struct));

	pdivoom_zlib_handle->safe_lock = divoom_system_create_lock();
	pdivoom_zlib_handle->zalloc = divoom_zlib_malloc;
	pdivoom_zlib_handle->zfree = divoom_zlib_free;
#endif
	return;
}

//dest不为空,source_len不能=0
//1success, -1 wrong
int divoom_zlib_decompress(uint8 *dest, DIVOOM_UINT_ID *dest_len, uint8 *source, DIVOOM_UINT_ID source_len)
{
	return 0;
#if 0
	int ret = 0;
	if(pdivoom_zlib_handle != NULL)
	{
		divoom_system_get_lock(pdivoom_zlib_handle->safe_lock);
		ret = (uncompress(dest, dest_len, source, source_len)>=0)?1:-1;
		divoom_system_free_lock(pdivoom_zlib_handle->safe_lock);
		return ret;
	}
	return -1;
#endif
}


#endif





