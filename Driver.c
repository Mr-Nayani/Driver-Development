#include <ntifs.h>
#include <fltKernel.h>

/*
==============================================================
Function prototypes
==============================================================
*/

// Driver-related
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
void FileDeleteDriverUnload(PDRIVER_OBJECT DriverObject);

// Filter-related
FLT_PREOP_CALLBACK_STATUS FileDeleteFilterPreOperation(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID* CompletionContext);
NTSTATUS FileDeleteFilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags);

/*
==============================================================
Global variables
==============================================================
*/

const FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_SET_INFORMATION,
      0,
      FileDeleteFilterPreOperation,
      NULL },
    { IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    NULL,
    Callbacks,
    FileDeleteFilterUnload,
    NULL // InstanceSetupCallback is not used.
};

PFLT_FILTER g_FilterHandle;

/*
==============================================================
Function implementations
==============================================================
*/

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;

    status = FltRegisterFilter(DriverObject, &FilterRegistration, &g_FilterHandle);   // Register the minifilter with the filter manager

    if (NT_SUCCESS(status))
    {
        status = FltStartFiltering(g_FilterHandle);   // Start filtering I/O operations

        if (!NT_SUCCESS(status))
        {
            FltUnregisterFilter(g_FilterHandle);   // Failed to start filtering, unregister the minifilter
            return status;
        }

        KdPrint(("DriverEntry: Filter started successfully\n"));

        DriverObject->DriverUnload = FileDeleteDriverUnload;   // Set driver unload callback
    }
    else
    {
        KdPrint(("DriverEntry: Failed to register filter. Status: 0x%X\n", status));
    }

    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS FileDeleteFilterPreOperation(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID* CompletionContext)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    if (Data != NULL && Data->Iopb != NULL)
    {
        if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
        {
            ULONG fileInfoClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

            if (fileInfoClass == FileDispositionInformation || fileInfoClass == FileDispositionInformationEx)
            {
                PFILE_DISPOSITION_INFORMATION dispositionInfo = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

                if (dispositionInfo != NULL && dispositionInfo->DeleteFile)
                {
                    KdPrint(("File deletion detected. Path: %wZ\n", &(FltObjects->FileObject -> FileName)));
                }
            }
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS FileDeleteFilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    if (g_FilterHandle != NULL)
    {
        FltUnregisterFilter(g_FilterHandle);
        g_FilterHandle = NULL;
    }

    KdPrint(("FileDeleteFilterUnload: Filter unloaded\n"));

    return STATUS_SUCCESS;
}

void FileDeleteDriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}