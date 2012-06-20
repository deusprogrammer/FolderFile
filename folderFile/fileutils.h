#ifndef FILEUTILS_H
#define FILEUTILS_H

#include "llist.h"
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

bool mkdir_m(char* filePath) {
#ifdef _WIN32
	return CreateDirectory(filePath, NULL);
#else
	return mkdir(filePath);
#endif
}

void ls_m(char* root, char* directoryPath, LList<char*>* files, LList<char*>* directories) {
	char fullPath[1024];
	char search[128];

#ifdef _WIN32
	HANDLE hFind;
	WIN32_FIND_DATA find;
	sprintf(search, "%s%s*", root, directoryPath);

	if((hFind = FindFirstFile(search, &find)) != INVALID_HANDLE_VALUE){
		do{
			if (strcmp(find.cFileName, ".") != 0 && strcmp(find.cFileName, "..") != 0) {
				if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					sprintf(fullPath, "%s%s", directoryPath, find.cFileName);

					char* copy = new char[1024];
					strcpy(copy, fullPath);
					directories->addToEnd(copy);

					sprintf(fullPath, "%s\\", fullPath);
					ls_m(root, fullPath, files, directories);
				}
				else {
					sprintf(fullPath, "%s%s", directoryPath, find.cFileName);

					char* copy = new char[1024];
					strcpy(copy, fullPath);
					files->addToEnd(copy);
				}
			}
		} while(FindNextFile(hFind, &find));

		FindClose(hFind);
	}
#else
	struct dirent *de=NULL;
	DIR *d = NULL;
	sprintf(search, "%s%s", root, directoryPath);

	d = opendir(search);
	if(d == NULL)
		return files;

	// Loop while not NULL
	while(de = readdir(d)) {
		switch(de->d_type) {
		case DT_DIR:
			if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
				sprintf(fullPath, "%s%s\\", directoryPath, de->d_name);
				files = ls_m(root, fullPath, files);
			}
			break;
		case DT_REG:
			sprintf(fullPath, "%s%s", directoryPath, de->d_name);
			printf("FILE %s\n", fullPath);
			files->addToEnd(fullPath);
			break;
		};
	}

	closedir(d);
#endif
}

#endif