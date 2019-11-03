#include "FileBase.h"

BOOLEAN FsCreateFile(UNICODE_STRING ustrFilePath)
{
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	//创建文件
	InitializeObjectAttributes(&objectAttributes, &ustrFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&hFile, GENERIC_READ, &objectAttributes, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}
	//关闭句柄
	ZwClose(hFile);
	return TRUE;
}


BOOLEAN FsCreateFolder(UNICODE_STRING ustrFolderPath)
{
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	//创建目录
	InitializeObjectAttributes(&objectAttributes, &ustrFolderPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&hFile, GENERIC_READ, &objectAttributes, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_DIRECTORY_FILE, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}
	ZwClose(hFile);
	return TRUE;
}


BOOLEAN FsDeleteFileOrFolder(UNICODE_STRING ustrFileName)
{
	NTSTATUS status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };

	InitializeObjectAttributes(&objectAttributes, &ustrFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	//执行删除操作
	status = ZwDeleteFile(&objectAttributes);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}
	return TRUE;
}


ULONG64 FsGetFileSize(UNICODE_STRING ustrFileName)
{
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	FILE_STANDARD_INFORMATION fsi = { 0 };

	//获取文件句柄
	InitializeObjectAttributes(&objectAttributes, &ustrFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&hFile, GENERIC_READ, &objectAttributes, &iosb, NULL, 0, FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		return 0;
	}
	//获取文件的大小
	status = ZwQueryInformationFile(hFile, &iosb, &fsi, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
	if (!NT_SUCCESS(status))
	{
		ZwClose(hFile);
		return 0;
	}

	ZwClose(hFile);
	return fsi.EndOfFile.QuadPart;
}


BOOLEAN FsRenameFileOrFolder(UNICODE_STRING ustrSrcFileName, UNICODE_STRING ustrDestFileName)
{
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	PFILE_RENAME_INFORMATION pRenameInfo = NULL;
	ULONG ulLenth = (1024 + sizeof(FILE_RENAME_INFORMATION));

	//申请内存
	pRenameInfo = (PFILE_RENAME_INFORMATION)ExAllocatePool(NonPagedPool, ulLenth);
	if (pRenameInfo == NULL)
	{
		return FALSE;
	}
	//设置重命名信息
	RtlZeroMemory(pRenameInfo, ulLenth);
	pRenameInfo->FileNameLength = ustrDestFileName.Length;
	wcsncpy(pRenameInfo->FileName, ustrDestFileName.Buffer, ustrDestFileName.Length);
	pRenameInfo->ReplaceIfExists = 0;
	pRenameInfo->RootDirectory = NULL;
	//设置源文件信息并获取句柄
	InitializeObjectAttributes(&objectAttributes, &ustrSrcFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(
		&hFile,
		SYNCHRONIZE | DELETE,
		&objectAttributes,
		&iosb,
		NULL,
		0,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
		NULL,
		0
		);

	if (!NT_SUCCESS(status))
	{
		ExFreePool(pRenameInfo);
		return FALSE;
	}
	//利用ZwSetInfomationFile来设置文件信息
	status = ZwSetInformationFile(hFile, &iosb, pRenameInfo, ulLenth, FileRenameInformation);
	if (!NT_SUCCESS(status))
	{
		ZwClose(hFile);
		ExFreePool(pRenameInfo);
		return FALSE;
	}
	//释放内存,关闭句柄
	ExFreePool(pRenameInfo);
	ZwClose(hFile);
	return TRUE;
}


BOOLEAN FsQueryFileAndFolder(UNICODE_STRING ustrPath)
{
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	NTSTATUS status = STATUS_SUCCESS;

	//获取文件句柄
	InitializeObjectAttributes(&objectAttributes, &ustrPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(
		&hFile,
		FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_ANY_ACCESS,
		&objectAttributes,
		&iosb,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN,
		FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
		NULL,
		0
		);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}

	//遍历文件
	//注意次数的大小,一定要申请足够的内存,否则后面ExFreePool会蓝屏
	ULONG ulLength = (2 * 4096 + sizeof(FILE_BOTH_DIR_INFORMATION)) * 0x2000;
	PFILE_BOTH_DIR_INFORMATION pDir = (PFILE_BOTH_DIR_INFORMATION)ExAllocatePool(PagedPool, ulLength);
	//保存pDir的首地址,用来释放内存使用
	PFILE_BOTH_DIR_INFORMATION pBeginAddr = pDir;
	//获取信息
	status = ZwQueryDirectoryFile(
		hFile, 
		NULL, 
		NULL, 
		NULL, 
		&iosb, 
		pDir, 
		ulLength,
		FileBothDirectoryInformation, 
		FALSE, 
		NULL, 
		FALSE
		);
	return TRUE;
}