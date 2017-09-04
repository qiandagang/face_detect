#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>
#include<curl/easy.h>
#include"cJSON.h"
#include "jsDevParamConfig.h"
#include<dirent.h>
#include<pthread.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/types.h>
#include<stdbool.h>
#include<sys/time.h>
#include<ctype.h>
#include<malloc.h>

#define BAD             -1
#define INVALID_ARG     -1
#define WRONG_FORMAT    -2
#define OUTPUT_OVERFLOW -3
//#define DevId       1
//#define DpId        1

int Curl_http(char *picPath,char *base64code,char *picName);
int GetPathFileName(char *path, char **pName, char **pOther);
long Get_pic_byte(char *srcpic,char *in,int size);

//static const char strResult[1024];
static char HTTPRESULT[1024] ;


typedef struct
{
    char gender[10];
    int age;
}person;

struct __dirstream   
{   
    void *__fd;    
    char *__data;    
    int __entry_data;    
    char *__ptr;    
    int __entry_ptr;    
    size_t __allocation;    
    size_t __size;       
 }; 
 
/***************************************************************************************
function:ascii_to_base64
param:  char *base64Char:һ��buffer���׵�ַ�����buffer���������������ת�������ݵġ�
        char *in:Դ�ַ������׵�ַ��ָ����Ҫת������ݣ���ԭ�ַ������ݡ�
        const long size:Դ�ַ����ĳ��ȡ�
****************************************************************************************/
static const char *ALPHA_BASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 
int base64_encode(const char *in, const long size, char *base64Char)
{
    int a = 0;
    int i = 0;
    //memset(base64Char, 0x00,strlen(base64Char)+1);

    while (i < size) {
        char b0 = in[i++];
        char b1 = (i < size) ? in[i++] : 0;
        char b2 = (i < size) ? in[i++] : 0;
         
        int int63 = 0x3F;  //  00111111
        int int255 = 0xFF; // 11111111
        base64Char[a++] = ALPHA_BASE[(b0 >> 2) & int63];
        base64Char[a++] = ALPHA_BASE[((b0 << 4) | ((b1 & int255) >> 4)) & int63];
        base64Char[a++] = ALPHA_BASE[((b1 << 2) | ((b2 & int255) >> 6)) & int63];
        base64Char[a++] = ALPHA_BASE[b2 & int63];
    }
    base64Char[a] = 0;     //�ַ���������־
    switch (size % 3) {
        case 1:
            base64Char[--a] = '=';
        case 2:
            base64Char[--a] = '=';
    }

    return 0;
}


//pic_address:Դ�ļ��ĵ�ַ���ļ��� file:Ŀ���ļ��ĵ�ַ
int copy_file(const char *dirpath_from, char *dirpath_to)   /*�ú���Ϊ������ͨ�ļ��ľ������*/
{
    int fd1,fd2,read_len,write_len;
    char buf[1024];
    char *pbuf;

    struct stat file;

    if(-1 == stat(dirpath_from,&file))                 /*stat�����ж��ļ���״̬ ͨ���ļ���dirpath_from��ȡ�ļ���Ϣ����������file��ָ�Ľṹ��stat��*/
    {                                                  /*stat()ִ�гɹ�����0��ʧ�ܷ���-1��������洢��errno��*/
        printf("Can not get info of %s\n",dirpath_from);
        return -1;
    }
    fd1 = open(dirpath_from,O_RDONLY);
    if(-1 == fd1)
    {
        printf("Can not open file dirpath_from:%s\n",dirpath_from);
        return -1;
    }
   /*
   ** access����:int access(const char * pathname, int mode)
   ** pathname:��Ҫ����ļ���·����;
   ** mode:��Ҫ���ԵĲ���ģʽ��
   ** R_OK:���Զ���ɡ�W_OK:����д��ɡ�X_OK:����ִ����ɡ�F_OK:�����ļ��Ƿ����
   ** ����ֵ:0��ʾ�ɹ���ʧ��:-1��
   */
    fd2 = open(dirpath_to,O_WRONLY|O_APPEND|O_CREAT,0666);

    if(-1 == fd2)
    {
        printf("open file error\n");
        close(fd1);
        return -1;
    }
    
    while((read_len = read(fd1,buf,1024)) >0 )
    {
        pbuf = buf;
        while((write_len = write(fd2,pbuf,read_len)) >0 )
        {
            pbuf += write_len;
            read_len -= write_len;
        }
        if(-1 == write_len)
        {
            printf("write file error\n");
            close(fd1);
            close(fd2);
            return -1;
        }
    }
    
    if(-1 == read_len)
    {
        printf("read error\n");
        close(fd1);
        close(fd2);
        return -1;
    }
    
    close(fd1);
    close(fd2);
    return 0;
}


int Get_age_region(char *base,char *region_dir,int size)
{
    char *pbase;
    
    pbase = base;
    pbase = strstr(base,"region");
    if(!pbase)
    {
        printf("file path is a error path!\n");
        return -1;
    }

    while((*pbase !=  '/') && (size-1))
    {
        *region_dir++ = *pbase++;
        --size;
    } 
    *region_dir = 0;

    return 0;
}


int Creat_file(char *buf,char *gen,char *pic_address,char *picName)
{
    char file[200]; 
    int ret;
    //struct stat file_stat;
    //struct timeval tv;
    /*
    ** �жϴ洢����ε�Ŀ¼�Ƿ���ڣ��������򴴽�
    */
    if(access(buf,F_OK))
    {
        if(mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            printf("mkdir:%s\n",strerror(errno));
        }
    }
    sprintf(file,"%s/%s",buf,gen);
    /*
    ** �жϴ洢�Ա�����Ŀ¼�Ƿ���ڣ��������򴴽�
    */ 
   /*
    * access����:int access(const char * pathname, int mode)
    * pathname:��Ҫ����ļ���·����;
    * mode:��Ҫ���ԵĲ���ģʽ��
    * R_OK:���Զ���ɡ�W_OK:����д��ɡ�F_OK:�����ļ��Ƿ����
    * ����ֵ:0��ʾ�ɹ���ʧ��:-1.
    */
    if(access(file,F_OK))
    {
        if(mkdir(file, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            printf("mkdir:%s\n",strerror(errno));
        }
    }
    /*
     * ����ͼƬ(ͼƬ��:��ʱ��ʱ�� ��_����)
     */
    sprintf(file,"%s/%s/%s",buf,gen,picName);
    printf("file:%s\n",file);

    ret = copy_file(pic_address,file);
    if(-1 == ret)
    {
        printf("copy picture failed!");
        return -1;
    }
 
    return 0;
}


int Picture_classfy(char *pic_address,person *person_info,char *picName)
{
    char buf[1024];
    char ptr[20];
    char age_region_dir[20];
    char dirpath[1024] = "./tclassfy_res";

    
    if(access(dirpath,F_OK))    //�ж�tfclassfy_res�Ƿ���ڣ��������򴴽�
    {
        if(mkdir(dirpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            printf("mkdir:%s\n",strerror(errno));
            return -1;
        }
    }

    if(person_info->age > 0 && person_info->age <= 80)   //�ܹ�ʶ���ͼƬ
    {
        sprintf(ptr,"region%d_%d",(((person_info->age-1)/5)*5+1),(((person_info->age-1)/5+1)*5));
        printf("ptr:%s\n",ptr);

        //printf("pic_address:%s\n",pic_address);
        memset(age_region_dir,0,sizeof(age_region_dir));
        Get_age_region(pic_address,age_region_dir,sizeof(age_region_dir));
        printf("age_region_dir:%s\n",age_region_dir);
        
        if(strcmp(age_region_dir,ptr))
        {
            printf("the picture is not accuracy!\n");
            return -2;
        }

        sprintf(buf,"%s/%s",dirpath,ptr);
        Creat_file( buf,person_info->gender,pic_address,picName);
    }
    else                                                //���ܹ�ʶ���ͼƬ
    {
        char file[200];
        //struct timeval tv;
        
        sprintf(ptr,"region_others");
        printf("ptr:%s\n",ptr);
        sprintf(buf,"%s/%s",dirpath,ptr);

        /*
         * �жϴ洢others��Ŀ¼�Ƿ���ڣ��������򴴽�
         */

        if(access(buf,F_OK)) //mode:F_OK,�����ļ��Ƿ���� 
        {
            if(mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
            {
                printf("mkdir:%s\n",strerror(errno));
                return -1;
            }
        }
        sprintf(file,"%s/%s",buf,picName);
#if 0
        gettimeofday(&tv, NULL);
        sprintf(file,"%s/%ld_%ld.jpg",buf,tv.tv_sec,tv.tv_usec);
#endif
        copy_file(pic_address,file);
    }
    
    return 0;
}

int dostat(char *path)
{
    int modify_time = 0;
    struct stat info;
    if(-1 == stat(path,&info))
    {
        printf("error:%s\n",strerror(errno));
        return -1;
    }
    
	modify_time = info.st_mtime;
	return modify_time;
}

int ImgProc(char *ImgPath,char *psLastImg)
{
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    char cbuf[1024];
    char *pName = NULL;
    char *ppsLastImg = NULL;
    int errflag;
    long inlen = 0;
    //int ret;
    char *out;
    static char in[1100000];

    if(NULL == (dir = opendir(ImgPath)))                   /*opendir������basePathĿ¼�����ѵ�ַ����ָ��dir*/
    {
        printf("open dir error...\n");
        return -1;
    }

    if(psLastImg)
    {
        GetPathFileName(psLastImg,&pName,&ppsLastImg);
        printf("=== %s, %s\n",pName,ppsLastImg);
    }

    while(NULL != (ptr = readdir(dir)))
    {
        dostat(ptr->d_name);
        if(NULL == ptr->d_name || 0 == strcmp(ptr->d_name,".") || 0 == strcmp(ptr->d_name,".."))
            continue;
        
        if(pName)                       /*��ĳ���ر��������п��ܴ������ݺ����ݶ�ʧpName��Ϊ�գ����ﲻ���ж�*/
        {
            if(strcmp(pName,ptr->d_name))
            {
                printf("   >> %s\n",ptr->d_name);
                continue;
            }
            printf("***>> %s <<***\n",ptr->d_name);
            pName = NULL;
        }
        sprintf(cbuf,"%s/%s",ImgPath,ptr->d_name);
        
        if(ptr->d_type == DT_REG)                        /*��ͨ�ļ� */
        {
            //if (NULL != strchr(ptr->d_name, '.') && 0 != strcmp(strchr(ptr->d_name, '.'), ".jpg"))
            if (0 == strstr(ptr->d_name, ".jpg"))
                continue;                                 //�������еķ�jpgͼƬ�ļ�

            JSDevParam_Set(1,1,"lastimg",cbuf);

            inlen = Get_pic_byte(cbuf,in,sizeof(in));     //��ȡͼƬ�Ķ����Ʊ��룬Ȼ�����base64����
            if(inlen <= 0)
            {
                printf("get pic byte error!");
                return -1;
            }
            out = (char *)malloc(sizeof(char)*inlen*1.5);
            if(NULL == out)
            {
                printf("malloc error!");
                return -1;
            }

            base64_encode(in,inlen,out);                            //base64����

            do{
                errflag = Curl_http(cbuf,out,ptr->d_name);          //�ϴ�ͼƬ��face++�ӿ�
            }while(-1 == errflag);
            
            free(out);
        }
        else if(ptr->d_type == DT_DIR)                  /*Ŀ¼�ļ� */
        {
            //printf("ptr->d_name:%s\n",ptr->d_name);
            ImgProc(cbuf, ppsLastImg);                    //�ݹ�
            ppsLastImg = NULL;
        }
    }
    closedir(dir);
    return 0;
}

int Get_itme_value(char *j_str,person *person_info)
{
    cJSON *root = NULL;
    cJSON *item = NULL;
    cJSON *pArrayItem = NULL;
    cJSON *gen = NULL;
    cJSON *age = NULL;
    cJSON *err = NULL;

    int nCount;
    char *out;
    root = cJSON_Parse(j_str);                                        //����value
    if(NULL == root)
    {
        printf("cJSON_Parse faild!");
        cJSON_Delete(root);
        return -1;
    }
    //printf("JSON_String:%s",j_str);
    out = cJSON_Print(root);
    printf("\nJSON_String:%s",out);
    free(out);
    
    memset(person_info,'\0',sizeof(person));

    err = cJSON_GetObjectItem(root,"error_msg");                      //��ȡerror_message�����ֵ����Ⲣ������
    if(NULL != err)
    {
        printf("------------%s----------\n",err->valuestring);
        if(root)
        {
            cJSON_Delete(root);                                       //�ͷ�cJSON)_Parse()����������ڴ�ռ�
            root = NULL;
        }
        return -1;
    }
    
    item = cJSON_GetObjectItem(root,"result");
    if(NULL != item)
    {
        nCount = cJSON_GetArraySize (item);                           //��ȡpArray����Ĵ�С(�����ĸ���)
        //printf("nCount:%d\n",nCount);
        for( int i = 0; i < nCount; i++)
        {
            pArrayItem = cJSON_GetArrayItem(item,i);                  //��ȡ����Ԫ��
            if(NULL == pArrayItem)
            {
                printf("pArrayItem getarrayitem fail!");
                cJSON_Delete(root);                                   //�ͷ�cJSON)_Parse()����������ڴ�ռ�
                return -1;
            }
            
            age = cJSON_GetObjectItem(pArrayItem,"age");                   //��ȡage����
            if(NULL == age)
            {
                printf("age getobject fail");
                cJSON_Delete(root);
                return -1;
            }
            person_info->age = age->valueint;   

        
            gen = cJSON_GetObjectItem(pArrayItem,"gender");                //��ȡgender����
            if(NULL == gen)
            {
                printf("gender getobjecet faild!");
                cJSON_Delete(root);                              //�ͷ�cJSON)_Parse()����������ڴ�ռ�
                return -1;
            }
            strncpy(person_info->gender,gen->valuestring,strlen(gen->valuestring)+1);
            //printf("\ngender:%s\n",person_info->gender);

        }
    }
    else 
    {
        printf("faces getobject fail!");
        cJSON_Delete(root);
        return -1;
    }
    printf("\nperson_info->gender:%s,person_info->age:%d\n",person_info->gender,person_info->age);

    if(root)
    {
        cJSON_Delete(root);                                       //�ͷ�cJSON)_Parse()����������ڴ�ռ�
        root = NULL;
    }

    if(0 == nCount)
        return 0;   //û������
    else if(1 == nCount)
        return 1;   //ֻ��һ������
    else 
        return 2;   //��������

    return 0;
}

long Get_pic_byte(char *srcpic, char *in, int size)
{
    long inlen = 0;
    int fd,read_len;
    fd = open(srcpic,O_RDONLY);
    if(-1 == fd)
    {
        printf("open error!");
        return -1;
    }
    inlen = lseek(fd,0,SEEK_END);
    if(inlen <= 0 || inlen > size)
    {
        printf("there are some error in lseek!\n");
        close(fd);
        return -1;
    }
    
    lseek(fd,0,SEEK_SET);
    read_len = read(fd,in,inlen);
    if(read_len != inlen)
    {
       printf("read faild!\n");
       close(fd);
       return -1;
    }

    close(fd);
    return read_len;
}


size_t Write_data(void* buffer, size_t size, size_t nmemb, void* user_p)                      
{                                                                                           //ptr��ָ��洢���ݵ�ָ��,size��ÿ����Ĵ�С
    //printf("***********size = %d,buffer = %s**********\n",(int)size,(char *)buffer);       //nmemb��ָ�����Ŀ,user_p���û�������
    size_t written; 
    written = size*nmemb;
    //printf("buffer:%s\n",(const char *)buffer);
    if(written < sizeof(HTTPRESULT))
    {
        strcpy(HTTPRESULT , (const char *)buffer);
        return written;
    }
    else
    {
        printf("the size of buffer exceeded HTTPRESULT!");
        return (size_t)-1;
    }
}

int Curl_http(char *picPath,char *base64code,char *picName)              //�ϴ�ͼƬ��face++�ӿ�
{
    CURL *curl = NULL;                            //����CURL����ָ��
    CURLcode res;                          //����CURLcode���͵ı��������淵��״̬��
    int ret;
    person people;
    static int i = 3;
    //printf("base64code:%s\n",base64code);
    struct curl_httppost *post=NULL;
    struct curl_httppost *last=NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(!curl)
    {
        printf("curl easy init fail!");
        sleep(1);
        return -1;
    }
    
    curl_formadd(&post,  
                 &last,  
                 CURLFORM_COPYNAME, "access_token",
                 CURLFORM_COPYCONTENTS, "24.040980ff81987bfe54724b97750aa98b.2592000.1505286034.282335-10003469",
                 CURLFORM_END);

    curl_formadd(&post,  
                 &last,  
                 CURLFORM_COPYNAME, "Content-Type",
                 CURLFORM_COPYCONTENTS, "application/x-www-form-urlencoded",
                 CURLFORM_END);  

    curl_formadd(&post,
                 &last, 
                 CURLFORM_COPYNAME, "image", 
                 CURLFORM_COPYCONTENTS,base64code,
                 CURLFORM_END);
    
    curl_formadd(&post,  
                 &last,  
                 CURLFORM_COPYNAME, "face_fields",  
                 CURLFORM_COPYCONTENTS, "gender,age",
                 CURLFORM_END);

    if(curl) 
    {
        /*��Ҫ����api��URL*/
        curl_easy_setopt(curl,CURLOPT_URL,"https://aip.baidubce.com/rest/2.0/face/v1/detect");
        
        curl_easy_setopt(curl,CURLOPT_HTTPPOST,post);
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
        
        memset(HTTPRESULT,'\0',sizeof(HTTPRESULT));               //��Ϣÿ�δ��䶼����ӣ�ÿ�ε��õ�ʱ��HTTPRESULT�������㣬����ᱨ��
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &HTTPRESULT);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Write_data);
        
        //curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );         //����Ļ��ӡ�������ӹ��̺ͷ���http����
        curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 0 );     //�ڷ�������ǰ�ȴ���ʱ�䣬����Ϊ0�����ʾ���޵ȴ�
        
        res = curl_easy_perform(curl);
        
        curl_easy_cleanup(curl);
        curl_formfree(post);
        
        if(res != CURLE_OK)
        {
            printf("curl_easy_perform()failed:%s,5s try again!\n",curl_easy_strerror(res));
            sleep(10);
            curl_global_cleanup();
            return -1;
        }
        //printf("%s",HTTPRESULT);     ����:{"error_message":"CONCURRENCY_LIMIT_EXCEEDED"}
        /*
          ѭ���ȴ���ֻҪ����face++�ӿ�ʱ����CONCURRENCY_LIMIT_EXCEEDED��
          �Ͳ�ͣ�ĵ��ã�ֱ����ӦΪֹ
        */ 
        sleep(1);
        printf("sendpath:%s\n",picPath);
        ret = Get_itme_value(HTTPRESULT,&people);                      /*����cjson���н���*/
        printf("ret:%d\n",ret);
        if(ret >= 0)                                                  //��ͼƬ�޷�ʶ��ʱ�ظ����3��
        {
            if(0 == ret)
            {
                while(i--)
                {
                    printf("there are no face in the picture\n");
                    printf("<^_^>repated:%d<^_^>\n",i);
                    printf("********************\n");
                    curl_global_cleanup();
                    return -1;
                }
                i = 3;
            }
            else if(1 == ret)
            {
                printf("just one face in the picture!");
            }
            else if(2 == ret)
            {
                people.age = 0; //�����ֶ�������ʱ����ͼƬ���ൽothers�ļ�����
                printf("there are many faces in the picture!\n");
            }
            i = 3;
        }
        else if(ret < 0)        //����ֵС��0����ʾ���ش�����Ϣ
        {
            printf("\nThere are something wrong,wait 2s try again...\n");
            sleep(2);
            curl_global_cleanup();
            return -1;
        }
        printf("\npeople.gender:%s,people.age:%d\n",people.gender,people.age);

        Picture_classfy(picPath,&people,picName);
        
        curl_global_cleanup();
    }
    return 0;
}

int GetPathFileName(char *path, char **pName, char **pOther)
{
    char *pTemp;

    if(path == NULL || !strlen(path))
    {
        *pName = NULL;
        *pOther = NULL;
        return -1;
    }
    
    while(*path == '/')
    {
        ++path;
    }
    pTemp = strstr(path,"/");
    if(pTemp)
    {   
        *pTemp = 0;                     //�ض��ַ�����ʱpath:��һ��Ŀ¼  
        *pOther = ++pTemp;
    }
    else
    {
        *pOther = NULL;
    }
    *pName = path;                     //*pName
    
    return 0;
}

int main(int argc, char *argv[])
{   
    char *ImgPath;
    char *psValue;
    char *psLastImg;
   
    if(argc > 1)
    {
        ImgPath = argv[1];  //�����·�����˹���
        printf("ImgPath:%s\n",ImgPath);
    }
    else
    {
        printf("Not set img path.\n");
        return -1;
    }
    
    if( 0 != JSDevParam_Open("./tfaceoutcfg.json"))
    {
        printf("faceoutcfg.json open failed!\n");
        return -1;
    }

    psValue = JSDevParam_Get(1,1,"respath");        //��ȡJSON�ļ���psValue == NULL,��ʾ��һ������
    if(!psValue)
    {
        JSDevParam_Set(1,1,"respath",ImgPath);      //ImgPath:����ʱ�����·��
    }
    else if(strcmp(psValue,ImgPath))                //��ֹ�����ж�ǰ���Ŀ¼��ƥ��
    {
        printf("Res path error\n");
        JSDevParam_Close();
        return -1;
    }
    psLastImg = JSDevParam_Get(1,1,"lastimg");
    if(psLastImg)
    {
        psLastImg = strstr(psLastImg,ImgPath);
        if(psLastImg)
        {
            psLastImg += strlen(ImgPath);
            printf("psLastImg:%s\n",psLastImg);
        }
        else
        {
            printf("psLastImg error\n");
        }
    }
    ImgProc(ImgPath,psLastImg);
    
    JSDevParam_Close();

    return 0;
}

