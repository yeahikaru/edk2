/** @file
  Brief Description of UEFI MyHelloWorld
  Detailed Description of UEFI MyHelloWorld
  Copyright for UEFI MyHelloWorld
  License for UEFI MyHelloWorld
**/

//#include <stdio.h>
#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/Smbios.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DevicePath.h>
#include <Register/Intel/Cpuid.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>
#include <Library/SmbusLib.h>
#include <Protocol/ShellParameters.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <IndustryStandard/Pci22.h>
#include <Protocol/Shell.h>
#include <Library/ShellLib.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>
#include <Library/PcdLib.h>

//#include <Protocol/PcdInfo.h>


//EFI_SYSTEM_TABLE* gST = NULL;
//EFI_BOOT_SERVICES* gBS = NULL;
//EFI_RUNTIME_SERVICES* gRT = NULL;

//extern EFI_GUID gEfiSmbiosProtocolGuid;

extern EFI_GUID gEfiSmbiosProtocalGuid;
extern EFI_GUID gEfiEdidActiveProtocolGuid;
extern EFI_GUID gEfiDevicePathProtocolGuid;
extern EFI_GUID gEfiGraphicsOutputProtocolGuid;
extern EFI_GUID gEfiPciIoProtocolGuid;
extern EFI_DRIVER_BINDING_PROTOCOL gPciBusDriverBinding;
extern EFI_GUID gEfiShellParametersProtocolGuid;
extern EFI_GUID gEfiShellProtocolGuid;
extern EFI_GUID gGetPcdInfoProtocolGuid;
extern EFI_GUID gEfiSimpleFileSystemProtocolGuid;

typedef struct {
	SMBIOS_STRUCTURE Hdr;
	UINT16 VendorId;
	UINT16 DeviceId;
} TEST_SMBIOS_TABLE;

TEST_SMBIOS_TABLE TestSmbiosTable = {
	{EFI_SMBIOS_TYPE_TEST_TABLE, sizeof(TEST_SMBIOS_TABLE), 0},
	0x1111,
	0x2222,
};

UINTN  Argc;
CHAR16** Argv;
UINT8 S_BUS;
UINT8 S_DEV;
UINT8 S_FUN;
volatile UINT64 gWorkerRuns = 0;



#define END_DEVICE_PATH_TYPE              0x7f
#define END_DEVICE_PATH                 END_DEVICE_PATH_TYPE
#define NODE_LENGTH(pPath) ((pPath)->Length[0]+((pPath)->Length[1]<<8))
#define SET_NODE_LENGTH(pPath,NodeLength) ((pPath)->Length[0]=(UINT8)(NodeLength),(pPath)->Length[1]=(NodeLength)>>8)
#define NEXT_NODE(pPath) ((EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)(pPath)+NODE_LENGTH(pPath)))
#define isEndNode(pDp) ((pDp)->Type==END_DEVICE_PATH)

VOID* DPGetLastNode(EFI_DEVICE_PATH_PROTOCOL* pDp)
{
	EFI_DEVICE_PATH_PROTOCOL* dp = NULL;
	//---------------------------------
	if (!pDp)return dp;
	for (; !isEndNode(pDp); pDp = NEXT_NODE(pDp))	dp = pDp;
	return 	dp;
}

EFI_DEVICE_PATH_PROTOCOL* DevicePath = NULL;

EFI_STATUS AddTestSmbiosTable(
	IN EFI_SMBIOS_PROTOCOL* EfiSmbiosProtocol, 
	IN UINTN Size, 
	IN EFI_SMBIOS_TABLE_HEADER* Record, 
	IN EFI_SMBIOS_HANDLE SmbiosHandle)
{
	EFI_STATUS Status;

	Size = TestSmbiosTable.Hdr.Length;
	Size += 2;
	Print(L"Size = %d\n", Size);
	Record = (EFI_SMBIOS_TABLE_HEADER*)AllocateZeroPool(Size);
	if (Record == NULL) {
		Print(L"Allocate Zero Pool Fail\n");
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem(Record, (UINT8*)&TestSmbiosTable, TestSmbiosTable.Hdr.Length);

	//SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

	Status = EfiSmbiosProtocol->Add(EfiSmbiosProtocol, NULL, &SmbiosHandle, Record);
	Print(L"Add TEST_SMBIOS_TABLE status = %r\n", Status);

	if (Status == EFI_SUCCESS) FreePool(Record);
	return Status;
	
}

CHAR8* DPType(UINT8 Type)
{
	switch (Type)
	{
		case 0x01:
			return "Type1 - Hardware Device Path";
			break;
		case 0x02:
			return "Type2 - ACPI Device Path";
			break;
		case 0x03:
			return "Type3 - Messaging Device Path";
			break;
		case 0x04:
			return "Type4 - Media Device Path";
			break;
		case 0x05:
			return "Type5 - BIOS Boot Specification Device Path";
			break;
		case 0x7F:
			return "Type6 - End Of Hardware Device Path";
			break;
		default:
			return "Out of Device Path Type";
			break;
	}
}

CHAR8* DP_Type1SubType(UINT8 SubType)
{
	switch (SubType)
	{
	case 1:
		return "Sub Type 1 - PCI";
		break;
	case 2:
		return "Sub Type 2 - PCCARD";
		break;
	case 3:
		return "Sub Type 3 - Memory Mapped";
		break;
	case 4:
		return "Sub Type 4 - Vendor";
		break;
	case 5:
		return "Sub Type 5 - Controller";
		break;
	case 6:
		return "BMC";
		break;
	default:
		return "Out of SubType";
		break;
	}
}

CHAR8* DP_Type2SubType(UINT8 SubType)
{
	switch (SubType)
	{
	case 1:
		return "Sub Type 1 - ACPI Device Path";
		break;
	case 2:
		return "Sub Type 2 - Expanded ACPI Device Path";
		break;
	case 3:
		return "Sub Type 3 - _ADR Device Path";
		break;
	case 4:
		return "Sub Type 4 - NVDIMM Device";
		break;
	default:
		return "Out of Sub Type";
		break;
	}
}

CHAR8* DP_Type3SubType(UINT8 SubType)
{
	switch (SubType)
	{
	case 1:
		return "Sub Type 1 - ATAPI";
		break;
	case 2:
		return "Sub Type 2 - SCSI";
		break;
	case 3:
		return "Sub Type 3 - Fibre Channel";
		break;
	case 4:
		return "Sub Type 4 - 1394";
		break;
	case 5:
		return "Sub Type 5 - USB";
		break;
	case 18:
		return "Sub Type 18 - SATA";
		break;
	case 16:
		return "Sub Type 16 - USB WWID";
		break;
	case 17:
		return "Sub Type 17 - Device Logical unit";
		break;
	case 15:
		return "Sub Type 15 - USB Class";
		break;
	case 6:
		return "Sub Type 6 - I2O Random Block Storage Class";
		break;
	case 11:
		return "Sub Type 11 - MAC Address for a network interface";
		break;
	case 12:
		return "Sub Type 12 - IPv4";
		break;
	case 13:
		return "Sub Type 13 - IPv6";
		break;
	case 20:
		return "Sub Type 20 - Vlan(802.1q)";
		break;
	case 9:
		return "Sub Type 9 - InfiniBand";
		break;
	case 14:
		return "Sub Type 14 - UART";
		break;
	case 10:
		return "Sub Type 10 - Vendor";
		break;
	case 21:
		return "Sub Type 21 - Fibre Channel Ex";
		break;
	case 22:
		return "Sub Type 22 - SAS Ex";
		break;
	case 19:
		return "Sub Type 19 - iSCSI";
		break;
	case 23:
		return "Sub Type 23 - NVM Express Namespace";
		break;
	case 24:
		return "Sub Type 24 - Universal Resource Identifier(URI) Device Path";
		break;
	case 25:
		return "Sub Type 25 - UFS";
		break;
	case 26:
		return "Sub Type 26 - SD";
		break;
	case 27:
		return "Sub Type 27 - Bluetooth";
		break;
	case 28:
		return "Sub Type 28 - Wi-Fi Device Path";
		break;
	case 29:
		return "Sub Type 29 - eMMC";
		break;
	case 30:
		return "Sub Type 30 - BluetoothLE";
		break;
	case 31:
		return "Sub Type 31 - DNS Device Path";
		break;
	case 32:
		return "Sub Type 32 - NVDIMM Namespace";
		break;
	case 33:
		return "Sub Type 33 - REST Service Device Path";
		break;
	case 34:
		return "Sub Type 34 - NVMe-oF Namespace Device Path";
		break;
	default:
		return "Out of Sub Type";
		break;
	}
}


CHAR8* DP_Type4SubType(UINT8 SubType)
{
	switch (SubType)
	{
	case 1:
		return "Sub Type 1 - Hard Drive";
		break;
	case 2:
		return "Sub Type 2 - CD-ROM ""EI Torito"" Format";
		break;
	case 3:
		return "Sub Type 3 - Vendor";
		break;
	case 4:
		return "Sub Type 4 - File Path";
		break;
	case 5:
		return "Sub Type 5 - Media Protocol";
		break;
	case 6:
		return "Sub Type 6 - PIWG Firmware File";
		break;
	case 7:
		return "Sub Type 7 - PIWG Firmware Volume";
		break;
	case 8:
		return "Sub Type 8 - Relative Offset Range";
		break;
	case 9:
		return "Sub Type 9 - RAM Dik Device Path";
		break;
	case 0xFF:
		return "End Entire Device Path";
		break;
	default:
		return "Out of Sub Type";
		break;
	}
}

CHAR8* DP_Type5SubType(UINT8 SubType)
{
	switch (SubType)
	{
	case 1:
		return "Sub Type 1 - BIOS Boot Specification Device Path";
		break;
	default:
		return "Out of Sub Type";
		break;
	}
}


CHAR8* DP_SubType(UINT8 Type, UINT8 SubType)
{
	switch (Type)
	{
	case 0x01:
		return DP_Type1SubType(SubType);
		break;
	case 0x02:
		return DP_Type2SubType(SubType);
		break;
	case 0x03:
		return DP_Type3SubType(SubType);
		break;
	case 0x04:
		return DP_Type4SubType(SubType);
		break;
	case 0x05:
		return DP_Type5SubType(SubType);
		break;
	default:
		return "Out Of Sub Type";
		break;
	}
}

EFI_STATUS SearchSmbiosTable(
	IN EFI_SMBIOS_PROTOCOL* EfiSmbiosProtocol,
	IN EFI_SMBIOS_HANDLE SmbiosHandle,
	IN EFI_SMBIOS_TABLE_HEADER* Record
	)
{
	EFI_STATUS Status;

	Print(L"%a\n", __FUNCTION__);
	//SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

	do {
		Status = EfiSmbiosProtocol->GetNext(EfiSmbiosProtocol, &SmbiosHandle, NULL, &Record, NULL);


		if (EFI_ERROR(Status))
		{
			Print(L"SmbiosProtocol->GetNext Status = %r\n", Status);
			break;
		}

		if (Record->Type == 127)
		{
			Print(L"Get SMBIOS Type 127!\n");
			break;
		}
	} while (Status == EFI_SUCCESS);
	return Status;
}

VOID DumpDevicePath(
	IN EFI_GUID* DevicePathProtocolGuid,
	IN UINTN NumberOfHandle,
	IN EFI_HANDLE* Handle)
{
	UINTN i;
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN OriAttr = 0;

	Print(L"Handle By Protocol - %g\n", DevicePathProtocolGuid);
	for (i = 0; i < NumberOfHandle; i++)
	{
		Status = gBS->HandleProtocol(
			Handle[i],
			DevicePathProtocolGuid,
			(VOID**)&DevicePath);
		//Print(L"HadnleBuffer[%d] - gEfiDevicePathProtocolGuid Status = %r\n", i, Status);

		if (!EFI_ERROR(Status))
		{
			OriAttr = gST->ConOut->Mode->Attribute;
			gST->ConOut->SetAttribute(gST->ConOut, EFI_YELLOW);
			Print(L"Handle[%d]: ", i);
			gST->ConOut->SetAttribute(gST->ConOut, OriAttr);
			Print(L"%a, %a\n", DPType(DevicePath->Type), DP_SubType(DevicePath->Type, DevicePath->SubType));

		}
	}
}

VOID HandleProtocolExam()
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN NumberOfHandles = 0;
	EFI_HANDLE* HandleBuffer;
	EFI_GUID** ProtocolGuidList;
	UINTN ProtocolCount = 0;
	UINTN i, j;
	
	UINTN EventIndex = 0;
	EFI_INPUT_KEY Key;

	Print(L"%a\n", __FUNCTION__);
	Status = gBS->LocateHandleBuffer(
							AllHandles,
							NULL,
							NULL,
							&NumberOfHandles,
							&HandleBuffer);
	if (!EFI_ERROR(Status))
	{
		Print(L"NumberOfHandles = %d\n", NumberOfHandles);
		for (i = 0; i < NumberOfHandles; i++)
		{
			Status = gBS->ProtocolsPerHandle(
							HandleBuffer[i],
							&ProtocolGuidList,
							&ProtocolCount);

			if (!(EFI_ERROR(Status)))
			{
				Print(L"Handle [%d]\n", i);
				//Print(L"%g\n", ProtocolGuidList);
				for (j = 0; j < ProtocolCount; j++)
				{
					Print(L"ProtocolGuid[%d] : %g\n", j, ProtocolGuidList[j]);
					
				}

				if ((i + 1) % 10 == 0) {
					Print(L"Press any key to continue or press 'q' to exit.\n");

					gBS->WaitForEvent(
									1,
									&gST->ConIn->WaitForKey,
									&EventIndex);
					Status = gST->ConIn->ReadKeyStroke(
													gST->ConIn,
													&Key);
					

					if (Key.UnicodeChar == 0x71) break;
				}
			}
		}
	}
}

VOID DumpCpuId()
{
	CPUID_BRAND_STRING_DATA CpuIdBrandStringEax;
	CPUID_BRAND_STRING_DATA CpuIdBrandStringEbx;
	CPUID_BRAND_STRING_DATA CpuIdBrandStringEcx;
	CPUID_BRAND_STRING_DATA CpuIdBrandStringEdx;
	CHAR8* ProcessorBuffer = NULL;
	CHAR8** Processor = &ProcessorBuffer;

	Print(L"%a\n", __FUNCTION__);

	// Allocate 49 bytes (48 for brand string + 1 for null terminator)
	*Processor = AllocateZeroPool(49);
	if (*Processor == NULL) {
		Print(L"Failed to allocate memory for CPU brand string\n");
		return;
	}

	AsmCpuid(CPUID_BRAND_STRING1, &CpuIdBrandStringEax.Uint32, &CpuIdBrandStringEbx.Uint32, &CpuIdBrandStringEcx.Uint32, &CpuIdBrandStringEdx.Uint32);
	CopyMem(*Processor + 0, &CpuIdBrandStringEax.BrandString, 4);
	CopyMem(*Processor + 4, &CpuIdBrandStringEbx.BrandString, 4);
	CopyMem(*Processor + 8, &CpuIdBrandStringEcx.BrandString, 4);
	CopyMem(*Processor + 12, &CpuIdBrandStringEdx.BrandString, 4);

	AsmCpuid(CPUID_BRAND_STRING2, &CpuIdBrandStringEax.Uint32, &CpuIdBrandStringEbx.Uint32, &CpuIdBrandStringEcx.Uint32, &CpuIdBrandStringEdx.Uint32);
	CopyMem(*Processor + 16, &CpuIdBrandStringEax.BrandString, 4);
	CopyMem(*Processor + 20, &CpuIdBrandStringEbx.BrandString, 4);
	CopyMem(*Processor + 24, &CpuIdBrandStringEcx.BrandString, 4);
	CopyMem(*Processor + 28, &CpuIdBrandStringEdx.BrandString, 4);

	AsmCpuid(CPUID_BRAND_STRING3, &CpuIdBrandStringEax.Uint32, &CpuIdBrandStringEbx.Uint32, &CpuIdBrandStringEcx.Uint32, &CpuIdBrandStringEdx.Uint32);
	CopyMem(*Processor + 32, &CpuIdBrandStringEax.BrandString, 4);
	CopyMem(*Processor + 36, &CpuIdBrandStringEbx.BrandString, 4);
	CopyMem(*Processor + 40, &CpuIdBrandStringEcx.BrandString, 4);
	CopyMem(*Processor + 44, &CpuIdBrandStringEdx.BrandString, 4);

	// Null-terminate the string
	(*Processor)[48] = 0;

	// Remove preceeding spaces
	while (**Processor == 0x20) {
		(*Processor)++;
	}

	Print(L"Processor Brand String : %a\n", *Processor);
	
	// Free the allocated memory
	FreePool(*Processor);
}

VOID Resolution()
{
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gGop = NULL;
	EFI_STATUS Status = EFI_SUCCESS;

	Print(L"%a\n", __FUNCTION__);
	Status = gBS->LocateProtocol(
							&gEfiGraphicsOutputProtocolGuid,
							NULL,
							(VOID**)&gGop);

	Print(L"Status = %r\n", Status);

	if (!EFI_ERROR(Status)) {
		Print(L"gGop->Mode->Info->HorizontalResolution = %d\n", gGop->Mode->Info->HorizontalResolution);
		Print(L"gGop->Mode->Info->VerticalResolution = %d\n", gGop->Mode->Info->VerticalResolution);
		Print(L"gGop->Mode.Mode = %d\n", gGop->Mode->Mode);
	}
	

}

VOID GetTimeExam()
{
	EFI_STATUS Status = EFI_SUCCESS;
	EFI_TIME time;

	Print(L"%a\n", __FUNCTION__);
	Status = gRT->GetTime(&time, NULL);
	if (!EFI_ERROR(Status))
	{
		Print(L"Month = %d, Day = %d, Year = %d.\n", time.Month, time.Day, time.Year);
	}
	else
	{
		Print(L"Status = %r\n", Status);
	}
}

VOID SmbiosExam(EFI_HANDLE *ImageHandle)
{
	EFI_STATUS Status = EFI_SUCCESS;
	STATIC EFI_SMBIOS_PROTOCOL* EfiSmbiosProtocol = NULL;
	EFI_SMBIOS_HANDLE SmbiosHandle = 0;
	EFI_SMBIOS_TABLE_HEADER* Record = NULL;
	UINTN Size = 0;

	Print(L"%a\n", __FUNCTION__);
	//Install EfiSmbiosProtocolInterface
	Status = gBS->InstallProtocolInterface(ImageHandle, &gEfiSmbiosProtocolGuid, EFI_NATIVE_INTERFACE, &EfiSmbiosProtocol);

	Print(L"Install SMBIOS Protocol status = %r\n", Status);

	if (!EFI_ERROR(Status))
	{
		//Locate EfiSmbiosProtocol
		Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&EfiSmbiosProtocol);
		Print(L"Locate SMBIOS Protocol status = %r\n", Status);
		if (!EFI_ERROR(Status))
		{
			//Obtain SMBIOS version
			Print(L"EfiSmbiosProtocol->MajorVersion = %d\n", EfiSmbiosProtocol->MajorVersion);
			Print(L"EfiSmbiosProtocol->MinorVersion = %d\n", EfiSmbiosProtocol->MinorVersion);

			SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

			Status = AddTestSmbiosTable(EfiSmbiosProtocol, Size, Record, SmbiosHandle);
			Status = SearchSmbiosTable(EfiSmbiosProtocol, SmbiosHandle, Record);

		}
	}
}

VOID EdidExam(EFI_HANDLE* ImageHandle)
{
	EFI_STATUS Status = EFI_SUCCESS;
	STATIC EFI_EDID_ACTIVE_PROTOCOL* EfiEdidActiveProtocol = NULL;
	UINT32* EdidDidVid = NULL;
	UINTN NumberOfHandle = 0;
	EFI_HANDLE* HandleBuffer = NULL;
	UINTN i = 0;
	ACPI_ADR_DEVICE_PATH* AcpiAdr;

	Print(L"%a\n", __FUNCTION__);

	Status = gBS->InstallProtocolInterface(
		ImageHandle,
		&gEfiEdidActiveProtocolGuid,
		EFI_NATIVE_INTERFACE,
		&EfiEdidActiveProtocol);

	Print(L"Install EfiEdidActiveProtcol Status = %r\n", Status);

	Status = gBS->LocateProtocol(
		&gEfiEdidActiveProtocolGuid,
		NULL,
		(VOID**)&EfiEdidActiveProtocol);

	Print(L"Locate EdidActiveProtcol Status = %r\n", Status);

	if (!EFI_ERROR(Status))
	{
		//Print(L"EfiEdidActiveProtocol->Edid[0x08] = 0x%02x\n", EfiEdidActiveProtocol->Edid[0x08]);
		//Print(L"EfiEdidActiveProtocol->Edid[0x09] = 0x%02x\n", EfiEdidActiveProtocol->Edid[0x09]);
		//Print(L"EfiEdidActiveProtocol->Edid[0x0A] = 0x%02x\n", EfiEdidActiveProtocol->Edid[0x0A]);
		//Print(L"EfiEdidActiveProtocol->Edid[0x0B] = 0x%02x\n", EfiEdidActiveProtocol->Edid[0x0B]);
		Print(L"Edid Pointer Addr= 0x%p\n", EfiEdidActiveProtocol->Edid);

		EdidDidVid = (UINT32*)&EfiEdidActiveProtocol->Edid[8];
		Print(L"0x%p\n", EdidDidVid);
		Print(L"Panel DidVid = 0x%x\n", *EdidDidVid);

		if (*EdidDidVid == 0x41A2AF06) {
			Print(L"Panel Check Pass\n");
		}

	}

	Status = gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiEdidActiveProtocolGuid,
		NULL,
		&NumberOfHandle,
		&HandleBuffer);
	//Print(L"LocateHandleBuffer status = %r\n", Status);

	if (!EFI_ERROR(Status)) {
		Print(L"NumberOfHandle : %d\n", NumberOfHandle);
		for (; i < NumberOfHandle; i++)
		{
			Print(L"Handle %d\n", i);

			Status = gBS->HandleProtocol(
				HandleBuffer[i],
				&gEfiDevicePathProtocolGuid,
				(VOID**)&DevicePath);

			Print(L"HandleProtocol - gEfiDevicePathProtocolGuid Status = %r\n", Status);

			AcpiAdr = DPGetLastNode(DevicePath);
			if (!AcpiAdr || (((AcpiAdr->ADR & 0xf00) != 0x400) && ((AcpiAdr->ADR & 0xf00) != 0x100))) {
				continue;
			}
			Status = gBS->HandleProtocol(
				HandleBuffer[i],
				&gEfiEdidActiveProtocolGuid,
				(VOID*)&EfiEdidActiveProtocol);

			Print(L"HandleProtocol - gEfiEdidActiveProtocolGuid Status = %r\n", Status);

			if (!EFI_ERROR(Status)) {
				Print(L"EfiEdidActiveProtocol->Edid[0] = 0x%x\n", EfiEdidActiveProtocol->Edid[0]);
				Print(L"EfiEdidActiveProtocol->Edid[1] = 0x%x\n", EfiEdidActiveProtocol->Edid[1]);
				Print(L"EfiEdidActiveProtocol->Edid[2] = 0x%x\n", EfiEdidActiveProtocol->Edid[2]);
				Print(L"EfiEdidActiveProtocol->Edid[3] = 0x%x\n", EfiEdidActiveProtocol->Edid[3]);
			}
		}
	}
}

VOID DevicePathExam(EFI_HANDLE *ImageHandle)
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN NumberOfHandle = 0;
	EFI_HANDLE* HandleBuffer = NULL;
	
	Print(L"%a\n", __FUNCTION__);
	Status = gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiDevicePathProtocolGuid,
		NULL,
		&NumberOfHandle,
		&HandleBuffer);
	Print(L"LocateHandleBuffer - gEfiDevicePathProtocolGuid Status = %r, NumberOfHandle = %d\n", Status, NumberOfHandle);

	if (!EFI_ERROR(Status))
	{
		DumpDevicePath(&gEfiDevicePathProtocolGuid, NumberOfHandle, HandleBuffer);
	}
}

EFI_STATUS PciIoExam(EFI_HANDLE Handle, BOOLEAN DumpBDF)
{
	EFI_STATUS Status = EFI_SUCCESS;

	EFI_PCI_IO_PROTOCOL* PciIo = NULL;
	UINTN HandleOfNumber;
	EFI_HANDLE *HandleBuffer;
	UINT8 i = 0;
	UINTN j = 0;
	UINTN k = 0;
	UINTN Segment = 0, Bus = 0, Dev = 0, Func = 0;
	//PCI_TYPE00 PciData;
	UINT8 PciData;
	UINTN OriAttr = 0;

	Print(L"%a\n", __FUNCTION__);

	Status = gBS->LocateHandleBuffer(
						ByProtocol,
						&gEfiPciIoProtocolGuid,
						NULL,
						&HandleOfNumber,
						&HandleBuffer);
	if (!EFI_ERROR(Status))
	{
		Print(L"LocateHandleBuffer - gEfiPciIoProtocolGuid\n");
		
		for (; i < HandleOfNumber; i++)
		{
			Status = gBS->HandleProtocol(
							HandleBuffer[i],
							&gEfiPciIoProtocolGuid,
							(VOID*)&PciIo);
			
			if (!EFI_ERROR(Status))
			{
				Print(L"********** Handle[%d] **********\n", i);

				Status = PciIo->GetLocation(
					PciIo,
					&Segment,
					&Bus,
					&Dev,
					&Func);
				
				if (!EFI_ERROR(Status)) {
					

					Print(L"Pci BUS : %d, DEV : %d, FUN :%d\n", Bus, Dev, Func);
					
					if (DumpBDF) continue;
/*
					Status = PciIo->Pci.Read(
						PciIo,
						EfiPciIoWidthUint8,
						0x0,
						sizeof(PCI_TYPE00),
						&PciData);
*/

					if (!EFI_ERROR(Status))
					{
						OriAttr = gST->ConOut->Mode->Attribute;

						for (j = 0; j <= 0xF0; j+= 0x10)
						{
							if (j == 0)
							{
								Print(L"\t\t\t");

								for (; j <= 0x0F; j++)
								{
									gST->ConOut->SetAttribute(
										gST->ConOut,
										EFI_YELLOW);

									Print(L"%02x ", j);

									gST->ConOut->SetAttribute(
										gST->ConOut,
										OriAttr);
								}
								Print(L"\n");
								j -= 0x10;
							}

							Status = gST->ConOut->SetAttribute(
														gST->ConOut,
														EFI_YELLOW);

							Print(L"%02x ", j);
							
							Status = gST->ConOut->SetAttribute(
														gST->ConOut,
														OriAttr);
														
							for (k = j; k <= j + 0x0F; k++)
							{
								//Print(L"%02x ", *(UINT8*)(&PciData + k));

								Status = PciIo->Pci.Read(
									PciIo,
									EfiPciIoWidthUint8,
									(UINT32)k,
									1,
									&PciData);

								Print(L"%02x ", PciData);
							}
							
							Print(L"\n");
						}

						Print(L"*********************************\n\n");
						//break;
					}
				}
			}
		}
	}
	return Status;
}

VOID SmbusExam(VOID)
{
	Print(L"%02x\n", 0x1234);
}

VOID PointerExam()
{
	UINT8 a[] = { 1, 2, 3, 4, 5 };
	UINT8* ptr = NULL;
	UINTN index = 0;
	ptr = a;

	Print(L"%a\n", __FUNCTION__);
	Print(L"a[] = {");
	for (; index < sizeof(a) / sizeof(a[0]); index++)
	{
		Print(L" %d ", a[index]);
	}
	Print(L"}\n\n");

	Print(L"&ptr = %p\n", &ptr);
	Print(L"a = %p\n", a);
	Print(L"&a = %p\n", &a);
	Print(L"&a[0] = %x\n", &a[0]);
	Print(L"&a[1] = %x\n", &a[1]);
	Print(L"*(UINT8*)&a[1] = %x\n", *(UINT8*)&a[1]);
	Print(L"*(ptr + 1) = %x\n", *(ptr + 1));
}

/**
  Allocate memory and clean it with zero.

  @param[in] Size   Size of memory to allocate.

  @return       Allocated address for output.

**/
VOID*
AllocateZeroPages(
	IN UINTN  Size
)
{
	VOID* Buffer;

	Buffer = AllocatePages(EFI_SIZE_TO_PAGES(Size));
	if (Buffer != NULL) {
		ZeroMem(Buffer, Size);
	}

	return Buffer;
}

VOID ReadFile()
{
	EFI_STATUS Status = EFI_SUCCESS;
	EFI_SHELL_PROTOCOL* ShellProtocol = NULL;
	SHELL_FILE_HANDLE ShellFileHandle;
	CHAR16* FileName = L"bios.bin";
	UINTN Size = 0;
	//UINT8* Buffer = NULL;
	UINTN i = 0;
	STATIC EFI_PHYSICAL_ADDRESS Buffer;
	
	Status = ShellOpenFileByName(FileName, &ShellFileHandle, EFI_FILE_MODE_READ, 0x0);

	Print(L"Open file : %s Status = %r\n", FileName, Status);
	if (!EFI_ERROR(Status)) {
		Status = ShellGetFileSize(ShellFileHandle, &Size);

		//Print(L"Size of Handle : 0x%x\n", Size);

		Status = gBS->LocateProtocol(
			&gEfiShellProtocolGuid,
			NULL,
			(VOID**)&ShellProtocol);

		Print(L"Locate ShellProtocol Status = %r\n", Status);

		if (!EFI_ERROR(Status))
		{
			Print(L"Read File : %s\n", FileName);
			Print(L"File Size : 0x%x\n", Size);
			Status = gBS->AllocatePages(
				AllocateAnyPages,
				EfiLoaderData,
				EFI_SIZE_TO_PAGES((UINTN)Size),
				&Buffer);

			Print(L"AllocatePages Status = %r\n", Status);

			if (!EFI_ERROR(Status)) {
				Status = ShellProtocol->ReadFile(
					ShellFileHandle,
					&Size,
					(VOID*)(UINTN)Buffer);

				Print(L"Read File Status = %r\n", Status);

				if (EFI_ERROR(Status)) {
					goto FreeMemory;
				}
				if (!EFI_ERROR(Status))
				{
					Print(L"Buffer : 0x%x, Size : %x\n", Buffer, Size);

					Print(L"0x%lx\n", *(EFI_PHYSICAL_ADDRESS*)(Buffer + 0xE0000));

					Print(L"0x%llx\n", *(unsigned long long*)(Buffer + 0xE0000));

					
					for (; i < 0x20; i++)
					{
						if ((i != 0) && (i % 0x10) == 0) Print(L"\n");
						Print(L"%02x\t\t", *(UINT8*)(EFI_PHYSICAL_ADDRESS*)(Buffer + 0xE0000 + i));
					}
					Print(L"\n");

					
				}
			}
FreeMemory:
	gBS->FreePages(Buffer, EFI_SIZE_TO_PAGES((UINTN)Size));
		}
	}
	/*
	{
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* SimpleFileSystemProtocol = NULL;
		EFI_FILE_PROTOCOL* FileProtocol = NULL;
//		EFI_FILE_HANDLE* FileHandle = NULL;
		EFI_FILE_PROTOCOL* TargetFileProtocol = NULL;

		Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&SimpleFileSystemProtocol);

		Print(L"SimpleFileSystemProtocol Status = %r\n", Status);

		if (!EFI_ERROR(Status))
		{
			Status = SimpleFileSystemProtocol->OpenVolume(
														SimpleFileSystemProtocol,
														&FileProtocol);

			Print(L"FileProtocol Status = %r\n", Status);

			if (!EFI_ERROR(Status))
			{
				Status = FileProtocol->Open(
										FileProtocol,
										&TargetFileProtocol,
										L"a.bin",
										EFI_FILE_MODE_READ,
										0);

				Print(L"FileProtocol->Open Status = %r\n", Status);

				if (!EFI_ERROR(Status))
				{
					//Status = ShellGetFileSize(ShellFileHandle, &Size);
					Size = 0xFF;

					Status = TargetFileProtocol->Read(
												FileProtocol,
												&Size,
												&Buffer);

					Print(L"FileProtocol->Read Status = %r\n", Status);
				}
			}
		}
	}
	*/
}

VOID EFIAPI CPUStressTestWorker(VOID* Buffer)
{
	UINT64 DummyReg = 0x123456789ABCDEFULL;
	CONST UINT64 StandardMod = 0xFFFFFFFFFFFFFFC5ULL;

	/* Instrumentation: count how many times the worker started */
	gWorkerRuns++;

	// Loop indefinitely
	while (TRUE) {
		DummyReg = (DummyReg * DummyReg) + 0xFEEDDEED;
		DummyReg ^= (DummyReg >> 21);
		DummyReg = DummyReg % StandardMod;
        
		// Optional: If you still want to track progress in a debugger or globally:
		// gTotalIterations++; 
	}
}

EFI_STATUS CpuStressTest()
{
	EFI_STATUS Status = EFI_SUCCESS;
	EFI_MP_SERVICES_PROTOCOL* MpServiceProtocol = NULL;
	UINTN NumberOfProcessors = 0;
	UINTN NumberOfEnabledProcessors = 0;
	EFI_EVENT APWaitEvent = NULL; // Event to handle asynchronous AP execution
	
	Print(L"%a\n", __FUNCTION__);
	Print(L"Initialize Continuous CPU Stress Test\n");
	Status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid, NULL, (VOID**)&MpServiceProtocol);
	if (EFI_ERROR(Status)) {
		Print(L"Unable to locate MP Service Protocol: %r. Falling back to BSP.\n", Status);
		CPUStressTestWorker(NULL);
		return EFI_SUCCESS;
	}

	Status = MpServiceProtocol->GetNumberOfProcessors(MpServiceProtocol, &NumberOfProcessors, &NumberOfEnabledProcessors);
	if (EFI_ERROR(Status)) {
		Print(L"Failed to get number of processors: %r. Falling back to BSP.\n", Status);
		CPUStressTestWorker(NULL);
		return EFI_SUCCESS;
	}

	Print(L"Processors: %lu, Enabled: %lu\n", (UINT64)NumberOfProcessors, (UINT64)NumberOfEnabledProcessors);

	if (NumberOfProcessors <= 1) {
		Print(L"Single processor system; running infinite stress on BSP.\n");
		CPUStressTestWorker(NULL);
		return EFI_SUCCESS;
	}

	// 1. Create a Wait Event to allow non-blocking execution
	Status = gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &APWaitEvent);
	if (EFI_ERROR(Status)) {
		Print(L"Failed to create event: %r. Falling back to BSP.\n", Status);
		CPUStressTestWorker(NULL);
		return EFI_SUCCESS;
	}

	// 2. Startup all APs asynchronously
	Status = MpServiceProtocol->StartupAllAPs(
		MpServiceProtocol,
		(EFI_AP_PROCEDURE)CPUStressTestWorker,
		FALSE,         // Single Thread (False means run on all APs concurrently)
		APWaitEvent,   // Passing a valid event forces non-blocking execution!
		0,             // Timeout (0 means infinite/no timeout)
		NULL,          // ProcedureArgument
		NULL           // Finished
	);

	if (EFI_ERROR(Status)) {
		Print(L"StartupAllAPs failed: %r. Falling back to BSP.\n", Status);
		gBS->CloseEvent(APWaitEvent);
		CPUStressTestWorker(NULL);
		return EFI_SUCCESS;
	}

	Print(L"APs started successfully. Launching worker on BSP...\n");

	// 3. Fire up the worker on the BSP as well. 
	// Since CPUStressTestWorker is now an infinite loop, this will never return.
	CPUStressTestWorker(NULL);

	// The code below will never realistically be reached unless you modify the worker loop to break.
	// But it is good practice to clean up resources.
	gBS->CloseEvent(APWaitEvent);
	return EFI_SUCCESS;
}

/**

  This function parse application ARG.

  @return Status
**/
static
EFI_STATUS
GetArg()
{
	EFI_STATUS                     Status;
	EFI_SHELL_PARAMETERS_PROTOCOL* ShellParameters = NULL;

	// Get ShellParameters protocol directly from the application's image handle
	Status = gBS->HandleProtocol(
		gImageHandle,
		&gEfiShellParametersProtocolGuid,
		(VOID**)&ShellParameters
	);

	if (FeaturePcdGet(MyAppEnableFeatureFlag)) {
		Print(L"[DEBUG] HandleProtocol on gImageHandle Status = %r\n", Status);
	}

	if (EFI_ERROR(Status)) {
		Print(L"[DEBUG] Unable to retrieve Shell Parameters Protocol from gImageHandle: %r\n", Status);
		Argc = 1;
		Argv = NULL;
		return EFI_SUCCESS;
	}

	if (FeaturePcdGet(MyAppEnableFeatureFlag)) {
		Print(L"[DEBUG] Successfully retrieved ShellParameters from gImageHandle\n");
		Print(L"[DEBUG] ShellParameters pointer = 0x%p\n", ShellParameters);
		Print(L"[DEBUG] ShellParameters->Argc = %d\n", ShellParameters->Argc);
		Print(L"[DEBUG] ShellParameters->Argv = 0x%p\n", ShellParameters->Argv);
	}

	if (ShellParameters->Argv == NULL) {
		Print(L"[DEBUG] Argv is NULL\n");
		Argc = 1;
		Argv = NULL;
		return EFI_SUCCESS;
	}

	if (FeaturePcdGet(MyAppEnableFeatureFlag)) {
		Print(L"[DEBUG] Argv[0] = %s\n", ShellParameters->Argv[0]);
		if (ShellParameters->Argc > 1) {
			Print(L"[DEBUG] Argv[1] = %s\n", ShellParameters->Argv[1]);
		}
		if (ShellParameters->Argc > 2) {
			Print(L"[DEBUG] Argv[2] = %s\n", ShellParameters->Argv[2]);
		}
	}

	Argc = ShellParameters->Argc;
	Argv = ShellParameters->Argv;
	
	if (FeaturePcdGet(MyAppEnableFeatureFlag)) {
		Print(L"[DEBUG] Final - Argc = %d, Argv = 0x%p\n", Argc, Argv);
	}
	
	return Status;
}

VOID PrintHelp()
{
	Print(L"MyHelloWorld \
		[Time] \
		[DevicePath] \
		[DumpCpuId] \
		[Resolution] \
		[PciIoExam bdf/all] \
		[SmbusExam] \
		[SmbiosExam] \
		[EdidExam] \
		[HandleProtocolExam] \
		[ShellReadFile] \
		[CpuStressTest]\n\n"
	);

	Print(L"Time  - GetTimeExam\n");
	Print(L"DevicePath  - DevicePathExam\n");
	Print(L"DumpCpuId  - DumpCpuId\n");
	Print(L"Resolution  - Resolution\n");
	Print(L"PciIoExam  - PciIoExam\n");
	Print(L"     -bdf    Print Bus/Dev/Func\n");
	Print(L"     -all    Print PCI Config\n");
	Print(L"SmbusExam  - SmbusExam\n");
	Print(L"SmbiosExam  - SmbiosExam\n");
	Print(L"EdidExam  - EdidExam\n");
	Print(L"HandleProtocolExam  - HandleProtocolExam\n");
	Print(L"ShellReadFile  - ShellReadFile\n");
	Print(L"CpuStressTest  - CpuStressTest\n");
}

/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE        ImageHandle,
	IN EFI_SYSTEM_TABLE* SystemTable
)
{
	EFI_STATUS Status = EFI_SUCCESS;
	//UINT8 num = 0;

	Print(L"Program %a Entry\n\n", __FUNCTION__);
	
	Status = GetArg();

	if (FeaturePcdGet(MyAppEnableFeatureFlag)) {
		Print(L"GetArg Status = %r\n", Status);
	}

	if (!EFI_ERROR(Status))
	{
		if (Argc == 1)
		{
			PrintHelp();
			return Status;
		}

		if (
			(StrCmp(Argv[1], L"-h") == 0) || 
			(StrCmp(Argv[1], L"-H") == 0) || 
			(StrCmp(Argv[1], L"-?") == 0) 
			)
		{
			PrintHelp();
			return Status;
		}
		else if (StrCmp(Argv[1], L"Time") == 0 || StrCmp(Argv[1], L"time") == 0)
		{
			GetTimeExam();
		}
		else if (StrCmp(Argv[1], L"DevicePath") == 0 || StrCmp(Argv[1], L"devicepath") == 0)
		{
			DevicePathExam(&ImageHandle);
		}
		else if (StrCmp(Argv[1], L"DumpCpuId") == 0 || StrCmp(Argv[1], L"dumpcpuid") == 0)
		{
			DumpCpuId();
		}
		else if (StrCmp(Argv[1], L"Resolution") == 0 || StrCmp(Argv[1], L"resolution") == 0)
		{
			Resolution();
		}
		else if (StrCmp(Argv[1], L"PciIoExam") == 0 || StrCmp(Argv[1], L"pcioioexam") == 0)
		{
			if (Argc != 3) return  EFI_INVALID_PARAMETER;

			//num = *(UINT8*)Argv[2];
			//Print(L"%x\n", num);
			if (StrCmp(Argv[2], L"bdf") == 0)
			{
				//Dump BDF only
				Status = PciIoExam(&ImageHandle, TRUE);

			}
			else if (StrCmp(Argv[2], L"all") == 0) {
				//Dump all PCI IO device
				Status = PciIoExam(&ImageHandle, FALSE);
			}

			else {
				return EFI_INVALID_PARAMETER;
			}
		}
		else if (StrCmp(Argv[1], L"SmbusExam") == 0 || StrCmp(Argv[1], L"smbusexam") == 0)
		{
			SmbusExam();
		}
		else if (StrCmp(Argv[1], L"PointerExam") == 0 || StrCmp(Argv[1], L"pointerexam") == 0)
		{
			PointerExam();
		}
		else if (StrCmp(Argv[1], L"SmbiosExam") == 0 || StrCmp(Argv[1], L"smbiosexam") == 0)
		{
			SmbiosExam(&ImageHandle);
		}
		else if (StrCmp(Argv[1], L"EdidExam") == 0 || StrCmp(Argv[1], L"edidexam") == 0)
		{
			EdidExam(&ImageHandle);
		}
		else if (StrCmp(Argv[1], L"HandleProtocolExam") == 0 || StrCmp(Argv[1], L"handleprotocolexam") == 0)
		{
			HandleProtocolExam();
		}
		else if (StrCmp(Argv[1], L"ShellReadFile") == 0 || StrCmp(Argv[1], L"shellreadfile") == 0)
		{
			ReadFile();
		}
		else if (StrCmp(Argv[1], L"CpuStressTest") == 0 || StrCmp(Argv[1], L"cpustresstest") == 0)
		{
			CpuStressTest();
		}
		else
		{
			return EFI_INVALID_PARAMETER;
		}
	}
	Print(L"\nProgram %a Exit - %r\n", __FUNCTION__, Status);
	

	return Status;
}