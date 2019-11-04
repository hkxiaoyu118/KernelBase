#pragma once

#include <ntifs.h>

BOOLEAN FsCreateFile(UNICODE_STRING ustrFilePath);				//创建文件
BOOLEAN FsCreateFolder(UNICODE_STRING ustrFolderPath);			//创建文件夹
BOOLEAN FsDeleteFileOrFolder(UNICODE_STRING ustrFileName);		//删除文件或文件夹(如果是文件夹的情况,文件夹必须为空,否则删除失败)
ULONG64 FsGetFileSize(UNICODE_STRING ustrFileName);				//获取指定文件的实际大小(单位字节)
BOOLEAN FsRenameFileOrFolder(UNICODE_STRING ustrSrcFileName, UNICODE_STRING ustrDestFileName); //重命名文件或文件夹
BOOLEAN FsQueryFileAndFolder(UNICODE_STRING ustrPath);			//遍历指定的文件夹