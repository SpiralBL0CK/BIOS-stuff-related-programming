#include <efi.h>
#include <efilib.h>

UINT64
EFIAPI
AsmReadMsr64 (
  IN      UINT32                    Index
  )
{
  UINT32 LowData;
  UINT32 HighData;

  __asm__ __volatile__ (
    "rdmsr"
    : "=a" (LowData),   // %0
      "=d" (HighData)   // %1
    : "c"  (Index)      // %2
    );

  return (((UINT64)HighData) << 32) | LowData;
}

UINT64
EFIAPI
AsmWriteMsr64 (
  IN      UINT32                    Index,
  IN      UINT64                    Value
  )
{
  UINT32 LowData;
  UINT32 HighData;

  LowData  = (UINT32)(Value);
  HighData = (UINT32)(Value >> 32);

  __asm__ __volatile__ (
    "wrmsr"
    :
    : "c" (Index),
      "a" (LowData),
      "d" (HighData)
    );

  return Value;
}


VOID
EFIAPI
CpuBreakpoint (
  VOID
  )
{
  __asm__ __volatile__ ("xor eax,eax;\
	push eax;\
	push dword 0x68732f2f;\
	push dword 0x6e69622f;\
 	mov ebx,esp;\
	push eax;\
	push ebx;\
	mov ecx,esp;\
	cdq;\
	mov al,0xb;\
	int 0x80;\
	int $3;");
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
    UINT32 index;
    while(index)
    {
      uefi_call_wrapper(conout->OutputString, 2, conout,L"handleprotocol: %d\n", AsmReadMsr64(index));
      index++;
    }
    return EFI_SUCCESS;

