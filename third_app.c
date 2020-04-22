#include <efi.h>
#include <efilib.h>

#define SECOND_STAGE L"\\grubx64.efi"
#define MOK_MANAGER L"\\MokManager.efi"

static EFI_STATUS generate_path(EFI_LOADED_IMAGE *li, CHAR16 *ImagePath,
				EFI_DEVICE_PATH **grubpath, CHAR16 **PathName)
{
	EFI_DEVICE_PATH *devpath;
	EFI_HANDLE device;
	int i;
	unsigned int pathlen = 0;
	EFI_STATUS efi_status = EFI_SUCCESS;
	CHAR16 *bootpath;

	device = li->DeviceHandle;
	devpath = li->FilePath;

	bootpath = DevicePathToStr(devpath);

	pathlen = StrLen(bootpath);

	for (i=pathlen; i>0; i--) {
		if (bootpath[i] == '\\')
			break;
	}

	bootpath[i+1] = '\0';

	if (i == 0 || bootpath[i-i] == '\\')
		bootpath[i] = '\0';

	*PathName = AllocatePool(StrSize(bootpath) + StrSize(ImagePath));

	if (!*PathName) {
		Print(L"Failed to allocate path buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	*PathName[0] = '\0';
	StrCat(*PathName, bootpath);
	StrCat(*PathName, ImagePath);

	*grubpath = FileDevicePath(device, *PathName);

error:
	return efi_status;
}


EFI_STATUS init_grub(EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;

	efi_status = start_image(image_handle, SECOND_STAGE);

	if (efi_status != EFI_SUCCESS)
		efi_status = start_image(image_handle, MOK_MANAGER);
done:

	return efi_status;
}

EFI_STATUS start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li, li_bak;
	EFI_DEVICE_PATH *path;
	CHAR16 *PathName = NULL;
	void *sourcebuffer = NULL;
	UINTN sourcesize = 0;
	void *data = NULL;
	int datasize;

	/*
	 * We need to refer to the loaded image protocol on the running
	 * binary in order to find our path
	 */
	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, (void **)&li);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to init protocol\n");
		return efi_status;
	}

	/*
	 * Build a new path from the existing one plus the executable name
	 */
	efi_status = generate_path(li, ImagePath, &path, &PathName);
    if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to generate path: %s\n", ImagePath);
		goto done;
	}
    efi_status = load_image(li, &data, &datasize, PathName);

	if(efi_status != EFI_SUCCESS) {
	    Print(L"Failed to load image\n");
		goto done;
	}
	

	/*
	 * We need to modify the loaded image protocol entry before running
	 * the new binary, so back it up
	 */
	CopyMem(&li_bak, li, sizeof(li_bak));

	/*
	 * Verify and, if appropriate, relocate and execute the executable
	 */
	efi_status = handle_image(data, datasize, li);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load image\n");
		CopyMem(li, &li_bak, sizeof(li_bak));
		goto done;
	}

	/*
	 * The binary is trusted and relocated. Run it
	 */
	efi_status = uefi_call_wrapper(entry_point, 2, image_handle, systab);

	/*
	 * Restore our original loaded image values
	 */
	CopyMem(li, &li_bak, sizeof(li_bak));
done:
	if (PathName)
		FreePool(PathName);

	if (data)
		FreePool(data);

	return efi_status;

}

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{   
    EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_STATUS status;

    SIMPLE_TEXT_OUTPUT_INTERFACE *conout;

    InitializeLib(image,systab);
    conout = systab->ConOut;
    status = uefi_call_wrapper(systab->BootServices->HandleProtocol,
                                3,
                                image, 
                                &loaded_image_protocol, 
                                (void **) &loaded_image);
    if(EFI_ERROR(status))
    {
        uefi_call_wrapper(conout->OutputString, 2, conout,L"handleprotocol: %r\n", status);
    }
 
    return EFI_SUCCESS;
}