#include <efi.h>
#include <efilib.h>

#include <../../../../../../kernel/arch/ia64/include/bootinfo.h>

#define KERNEL_LOAD_ADDRESS 0x4400000

#define MEM_MAP_DESCRIPTOR_OFFSET_TYPE 0	    
#define MEM_MAP_DESCRIPTOR_OFFSET_BASE 8    
#define MEM_MAP_DESCRIPTOR_OFFSET_PAGES	24    



//Link image as a data array into hello - usefull with network boot
#define IMAGE_LINKED

bootinfo_t *bootinfo=(bootinfo_t *)BOOTINFO_ADDRESS;


#ifdef IMAGE_LINKED
extern char HOSimage[];
extern int HOSimagesize;
#endif



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

	char FileName[1024];
	char *OsKernelBuffer;
	int i;
	int defaultLoad;
	int imageLoad;
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
	
	if(LoadOptions[i++]==0){
		StrCat(FileName,L"\\image.bin");
		defaultLoad=1;
	}	
	else{
		CHAR16 buf[1024];
		buf[0]='\\';
		i--;
		int j;
		for(j=0;LoadOptions[i+j]!=L' '&&LoadOptions[i+j]!=0;j++)
			buf[j+1]=LoadOptions[i+j];
		buf[j+1]=0;
		StrCat(FileName,buf);
		defaultLoad=0;
	}

	imageLoad=1;
#ifdef IMAGE_LINKED
	if(defaultLoad) {
	    Print(L"Using Linked Image\n");
	    imageLoad=0;
	}    
#endif	
	

	char *  HOS;
	if(imageLoad)
	{
		Size = 0x00400000;

    		Vol->OpenVolume (Vol, &CurDir);

		EFI_STATUS stat;
		stat=CurDir->Open(CurDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
		if(EFI_ERROR(stat)){
			Print(L"Error Opening Image %s\n",FileName);
			return 0;
		}
	        BS->AllocatePool(EfiLoaderData, Size, &OsKernelBuffer);
		FileHandle->Read(FileHandle, &Size, OsKernelBuffer);
		FileHandle->Close(FileHandle);
		HOS = OsKernelBuffer;  
	    	if(Size<1) return 0;

	}	    
#ifdef IMAGE_LINKED
	else {
	    HOS = HOSimage;  
	    Size = HOSimagesize;
	    Print(L"Image start %llX\n",(long long)HOS);
	    Print(L"Image size %llX\n",(long long)Size);
	    Print(L"Image &size %llX\n",(long long)&Size);
	}
#endif	
	int HOSSize = Size;  


	rArg rSAL;
	rArg rPAL;

	//Setup AP's wake up address
	LibSalProc(0x01000000,2,0x4400200,0,0,0,0,0,&rSAL);


	//Get System Frequency
	UINT64 sys_freq;
	LibSalProc(0x01000012,0,0,0,0,0,0,0,&rSAL);
	sys_freq=rSAL.p1;
	

	UINT64 freq_scale;
	//Get CPU Frequency to System Frequency ratio
	LibPalProc(14,0,0,0,&rPAL);
	freq_scale=rPAL.p1;


        UINT64 sapic;
        LibGetSalIpiBlock(&sapic);
        Print (L"SAPIC:%X\n", sapic);
	//bootinfo->sapic=sapic;


        UINT64 wakeup_intno;
		LibGetSalWakeupVector(&wakeup_intno);
        Print (L"WAKEUP INTNO:%X\n", wakeup_intno);





	    UINTN cookie;
	    void *p=(void *)KERNEL_LOAD_ADDRESS;
	    UINTN mapsize,descsize;
	    UINT32 desver;
	    EFI_STATUS status;
	    
	        	    
	    
	    status=BS->AllocatePages(AllocateAnyPages,EfiLoaderData,/*(HOSSize>>12)+1*/ 1,p);
	    if(EFI_ERROR(status)){
		Print(L"Error 0\n");
		if(status == EFI_OUT_OF_RESOURCES) Print(L"EFI_OUT_OF_RESOURCES\n");
		if(status == EFI_INVALID_PARAMETER) Print(L"EFI_INVALID_PARAMETER\n");
		if(status == EFI_NOT_FOUND) Print(L"EFI_NOT_FOUND\n");
		return EFI_SUCCESS;
	    }
	    
	    UINTN no_entryes;
	    void * mds;
	    mds=LibMemoryMap(&no_entryes,&cookie,&descsize,&desver);
	    
	    for(i=0;i<no_entryes;i++)
	    {
	    
	    	unsigned int type=*((unsigned int *)(mds+i*descsize+MEM_MAP_DESCRIPTOR_OFFSET_TYPE));
	    	unsigned long long base=*((unsigned long long *)(mds+i*descsize+MEM_MAP_DESCRIPTOR_OFFSET_BASE));
	    	unsigned long long pages=*((unsigned long long *)(mds+i*descsize+MEM_MAP_DESCRIPTOR_OFFSET_PAGES));
	    	Print(L"T:%02d %016llX %016llX\n",type,base,pages*EFI_PAGE_SIZE);
		
	    }
	    status=BS->ExitBootServices(image,cookie);	
	    if(EFI_ERROR(status)){
		Print(L"Error 2\n");
		return EFI_SUCCESS;
	    }
	    
	
	int a;
	
	for(a=0;a<HOSSize;a++){
	    ((char *)(0x4400000))[a]=HOS[a];
	}
	bootinfo->sapic=(unsigned long *)sapic;
	bootinfo->wakeup_intno=wakeup_intno;
	bootinfo->sys_freq=sys_freq;
	bootinfo->freq_scale=freq_scale;
	bootinfo->hello_configured=1;


	    bootinfo->memmap_items=0;
	    
	    for(i=0;i<no_entryes;i++)
	    {
	    
	    	unsigned int type=*((unsigned int *)(mds+i*descsize+MEM_MAP_DESCRIPTOR_OFFSET_TYPE));
	    	unsigned long long base=*((unsigned long long *)(mds+i*descsize+MEM_MAP_DESCRIPTOR_OFFSET_BASE));
	    	unsigned long long pages=*((unsigned long long *)(mds+i*descsize+MEM_MAP_DESCRIPTOR_OFFSET_PAGES));
	    	
	    	
	    	
	    	switch (type)
	    	{
	    		case EfiConventionalMemory:
				bootinfo->memmap[bootinfo->memmap_items].type=EFI_MEMMAP_FREE_MEM;   	
				bootinfo->memmap[bootinfo->memmap_items].base=base;   	
				bootinfo->memmap[bootinfo->memmap_items].size=pages*EFI_PAGE_SIZE;   	
	    			bootinfo->memmap_items++;
	    			break;
	    		case EfiMemoryMappedIO:
				bootinfo->memmap[bootinfo->memmap_items].type=EFI_MEMMAP_IO;   	
				bootinfo->memmap[bootinfo->memmap_items].base=base;   	
				bootinfo->memmap[bootinfo->memmap_items].size=pages*EFI_PAGE_SIZE;   	
	    			bootinfo->memmap_items++;
	    			break;
	    		case EfiMemoryMappedIOPortSpace:
				bootinfo->memmap[bootinfo->memmap_items].type=EFI_MEMMAP_IO_PORTS;   	
				bootinfo->memmap[bootinfo->memmap_items].base=base;   	
				bootinfo->memmap[bootinfo->memmap_items].size=pages*EFI_PAGE_SIZE;   	
	    			bootinfo->memmap_items++;
	    			break;
	    			
	    		default :
	    			break;
	    	}
	    	
	    	
	    	
	    	

	    }


	
	//Run Kernel
	asm volatile(	
		"nop.i 0x00 ;;\n"
		"movl r15 = 0x4400000 ;;\n"
		"mov b0 = r15;;"
		"br.few b0;;\n"
	);
	   
	
	//Not reached	   
	return EFI_SUCCESS;
}
