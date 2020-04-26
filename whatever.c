/*DUMBASS IN'S NOT EDK2 STACK IF GNU-EFI STACK.... SRY IF I MISGUDED U WITH THE COMMENT */
#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "shim.h"
#include "netboot.h"

/*======================CONSTANT DEFINITION===================*/
static EFI_IP_ADDRESS tftp_addr;
static EFI_PXE_BASE_CODE *pxe;
void *sourcebuffer = NULL;



EFI_STATUS FetchUPD(EFI_HANDLE image_handle, VOID **buffer, UINTN *bufsiz)
{
    EFI_STATUS rc;
	EFI_PXE_BASE_CODE_TFTP_OPCODE read = EFI_PXE_BASE_CODE_TFTP_READ_FILE;
	BOOLEAN overwrite = FALSE;
	BOOLEAN nobuffer = FALSE;
	UINTN blksz = 512;

	Print(L"Fetching Netboot Image\n");
	if (*buffer == NULL) {
		*buffer = AllocatePool(4096 * 1024);
		if (!*buffer)
			return EFI_OUT_OF_RESOURCES; 
		*bufsiz = 4096 * 1024;
	}
try_again:
	rc = uefi_call_wrapper(pxe->Mtftp, 10, pxe, read, *buffer, overwrite,
				&bufsiz, &blksz, &tftp_addr, full_path, NULL, nobuffer);

	if (rc == EFI_BUFFER_TOO_SMALL) {
		/* try again, doubling buf size */
		*bufsiz *= 2;
		FreePool(*buffer);
		*buffer = AllocatePool(*bufsiz);
		if (!*buffer)
			return EFI_OUT_OF_RESOURCES;
		goto try_again;
	}

	return rc;
}

BOOLEAN findNetboot(EFI_HANDLE image_handle)
{
    UINTN bs = sizeof(EFI_HANDLE);
	EFI_GUID pxe_base_code_protocol = EFI_PXE_BASE_CODE_PROTOCOL;
	EFI_HANDLE *hbuf;
	BOOLEAN rc = FALSE;
	void *buffer = AllocatePool(bs);
	UINTN errcnt = 0;
	UINTN i;
	EFI_STATUS status;

	if (!buffer)
		return FALSE;

try_again:
	status = uefi_call_wrapper(BS->LocateHandle,5, ByProtocol, 
				   &pxe_base_code_protocol, NULL, &bs,
				   buffer);

	if (status == EFI_BUFFER_TOO_SMALL) {
		errcnt++;
		FreePool(buffer);
		if (errcnt > 1)
			return FALSE;
		buffer = AllocatePool(bs);
		if (!buffer)
			return FALSE;
		goto try_again;
	}
    if (status == EFI_NOT_FOUND) {
		FreePool(buffer);
		return FALSE;
	}
    hbuf = buffer;
    pxe = NULL;
    for(i = 0 ; i (bs / sizeof(EFI_HANDLE)); i++)
    {
        status = uefi_call_wrapper(BS->OpenProtocol, 6, hbuf[i],
			&pxe_base_code_protocol,
			&pxe, image_handle, NULL,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (status != EFI_SUCCESS) {
			pxe = NULL;
			continue;
		}

    }
}

static EFI_STATUS parseDhcp4()
{
	char *template = "/grubx64.efi";
	char *tmp = AllocatePool(16);


	if (!tmp)
		return EFI_OUT_OF_RESOURCES;


	memcpy(&tftp_addr.v4, pxe->Mode->DhcpAck.Dhcpv4.BootpSiAddr, 4);

	memcpy(tmp, template, 12);
	tmp[13] = '\0';
	full_path = tmp;

	/* Note we don't capture the filename option here because we know its shim.efi
	 * We instead assume the filename at the end of the path is going to be grubx64.efi
	 */
	return EFI_SUCCESS;
}


EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
    /*declaration for neccessary fct call*/
    void *data;
    void * sourcebuffer;
    UINT sourcesize;
    int datasize;

    InitializeLib(image_handle, systab);
    if (findNetboot(image_handle)) {
       if(parseDhcp4())
       {
        efi_status = FetchNetbootimage(image_handle, &sourcebuffer,
					       &sourcesize);
		if (efi_status != EFI_SUCCESS) {
			Print(L"Unable to fetch do UDP READ image\n");
			return efi_status;
		}
        }
    }
    data = sourcebuffer;
    datasize = sourcesize;
    return efi_status;
}
