#include <efi.h>
#include <efilib.h>

#define KERNEL_LOAD_ADDRESS 0x4400000


static CHAR16 *
a2u (char *str)
{
	static CHAR16 mem[2048];
	int i;

	for (i = 0; str[i]; ++i)
		mem[i] = (CHAR16) str[i];
	mem[i] = 0;
	return mem;
}
char HEX[256];

char hexs[]="0123456789ABCDEF";
/*
void to_hex(unsigned long long num)
{
    int a;
    for(a=15;a>=0;a--)    
    {
	char c=num - (num & 0xfffffffffffffff0LL);
	num/=16;
	c=hexs[c];
	HEX[a]=c;
    }

}
*/

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *conout;

	EFI_INPUT_KEY efi_input_key;
	EFI_STATUS efi_status;

	InitializeLib(image, systab);

	Print(L"HelloLib application started\n");

	EFI_GUID LoadedImageProtocol=LOADED_IMAGE_PROTOCOL;
	EFI_GUID DevicePathProtocol=DEVICE_PATH_PROTOCOL;
	EFI_GUID FileSystemProtocol=SIMPLE_FILE_SYSTEM_PROTOCOL;
	
	
	EFI_LOADED_IMAGE *LoadedImage;
	EFI_DEVICE_PATH *DevicePath;
	
	BS->HandleProtocol(image, 
	&LoadedImageProtocol, 
	&LoadedImage);
	BS->HandleProtocol(LoadedImage->DeviceHandle, 
	&DevicePathProtocol, 
	&DevicePath);
	Print (L"Image device : %s\n", DevicePathToStr (DevicePath));
	Print (L"Image file   : %s\n", DevicePathToStr (LoadedImage->FilePath));
	Print (L"Image Base   : %X\n", LoadedImage->ImageBase);
	Print (L"Image Size   : %X\n", LoadedImage->ImageSize);



	EFI_FILE_IO_INTERFACE *Vol;

	EFI_FILE *CurDir;
	EFI_FILE *FileHandle;

	BS->HandleProtocol(LoadedImage->DeviceHandle, &FileSystemProtocol, &Vol);
	Vol->OpenVolume (Vol, &CurDir);

	char FileName[1024];
	char *OsKernelBuffer;
	int i;
	UINTN Size;

	StrCpy(FileName,DevicePathToStr(LoadedImage->FilePath));
	for(i=StrLen(FileName);i>=0 && FileName[i]!='\\';i--);
	FileName[i] = 0;
	
	Print(L"%s\n",LoadedImage->LoadOptions);
	
	i=0;
	CHAR16 *LoadOptions = LoadedImage->LoadOptions;
	
	
	
	while(1) if(LoadOptions[i++]!=L' ') break;
	while(LoadOptions[i]!=L' '){	
		if(LoadOptions[i]==0) break;
		i++;
	}
	while(LoadOptions[i]==L' ') if(LoadOptions[i++]==0) break;
	
	if(LoadOptions[i++]==0)
		StrCat(FileName,L"\\image.bin");
	else{
		CHAR16 buf[1024];
		buf[0]='\\';
		i--;
		int j;
		for(j=0;LoadOptions[i+j]!=L' '&&LoadOptions[i+j]!=0;j++)
			buf[j+1]=LoadOptions[i+j];
		buf[j+1]=0;
		StrCat(FileName,buf);
	}
	
	//Print(L"%s\n",FileName);

	EFI_STATUS stat;
	stat=CurDir->Open(CurDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
	if(EFI_ERROR(stat)){
		Print(L"Error Opening Image %s\n",FileName);
		return 0;
	}    
	Size = 0x00400000;
	BS->AllocatePool(EfiLoaderData, Size, &OsKernelBuffer);
	FileHandle->Read(FileHandle, &Size, OsKernelBuffer);
	FileHandle->Close(FileHandle);

	if(Size<1) return 0;


	char *  HOS = OsKernelBuffer;  
	int HOSSize = Size;  


	{
	    UINTN cookie;
	    void *p=(void *)KERNEL_LOAD_ADDRESS;
	    UINTN mapsize,descsize;
	    UINT32 desver;
	    EFI_STATUS status;
	    EFI_MEMORY_DESCRIPTOR emd[1024];
	    
	        	    
	    mapsize=1024*sizeof(emd);
	    
	    status=BS->AllocatePages(AllocateAnyPages,EfiLoaderData,/*(HOSSize>>12)+1*/ 1,p);
	    if(EFI_ERROR(status)){
		Print(L"Error 0\n");
		if(status == EFI_OUT_OF_RESOURCES) Print(L"EFI_OUT_OF_RESOURCES\n");
		if(status == EFI_INVALID_PARAMETER) Print(L"EFI_INVALID_PARAMETER\n");
		if(status == EFI_NOT_FOUND) Print(L"EFI_NOT_FOUND\n");
		return EFI_SUCCESS;
	    }
	    
	    status=BS->GetMemoryMap(&mapsize,emd,&cookie,&descsize,&desver);
	    if(EFI_ERROR(status)){
		Print(L"Error 1\n");
		return EFI_SUCCESS;
	    }
	    status=BS->ExitBootServices(image,cookie);	
	    if(EFI_ERROR(status)){
		Print(L"Error 2\n");
		return EFI_SUCCESS;
	    }
	    
	}
	int a;
	
	for(a=0;a<HOSSize;a++){
	    ((char *)(0x4400000))[a]=HOS[a];
	}
	
	//Run Kernel
	asm volatile(	
		"nop.i 0x00 ;;\n"
		"movl r15 = 0x4400000 ;;\n"
		"mov b0 = r15;;"
		"br.few b0;;\n"
	);
	   
	
	while(1){
	    ((volatile int *)(0x80000000000b8000))[0]++;
	}
	return EFI_SUCCESS;
}
