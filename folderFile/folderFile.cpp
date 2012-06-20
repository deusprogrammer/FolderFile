// folderFile.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fileutils.h"
#include "llist.h"

struct FileHeader {
	char filePath[32];
	unsigned int size;
	unsigned int dataOffset;
};

class FolderFile {
protected:
	LList<FileHeader*> headers;
	LList<char*> directories;
	int nextOffset;
	char root[128];
	bool compressed;
	bool decompressed;

	void addFile(char* path);
	void addFolder(char* path);
	bool unify(char* filePath, bool append = false);
public:
	FolderFile() {nextOffset = 0; compressed = decompressed = false;}
	FolderFile(char* root) {strcpy(this->root, root); nextOffset = 0; compressed = decompressed = false;}

	bool compressDirectory(char* folder, char* destination);
	bool decompressDirectory(char* filePath, char* destination);
};

void FolderFile::addFile(char* path) {
	FileHeader* fh;
	char completePath[128];

	sprintf(completePath, "%s%s", this->root, path);

	struct _stat buffer;

	if (_stat(completePath, &buffer) != 0)
		return;

	fh = new FileHeader();
	strcpy(fh->filePath, path);
	fh->dataOffset = this->nextOffset;
	fh->size = buffer.st_size;
	this->nextOffset += fh->size;
	headers.addToEnd(fh);
}

void FolderFile::addFolder(char* path) {
	directories.addToEnd(path);
}

bool FolderFile::unify(char* path, bool append) {
	FILE* fp1 = fopen(path, "wb");
	FileHeader* p;
	int nFiles = headers.getSize();
	int nDirectories = directories.getSize();
	char completePath[128];

	if (!fp1)
		return false;

	fprintf(fp1, "\x09\xF9\x11\x02");
	fwrite(&nFiles, sizeof(unsigned int), 1, fp1);
	fwrite(&nDirectories, sizeof(unsigned int), 1, fp1);

	headers.moveCursor(HEAD);
	while (p = headers.stepForward()) {
		printf("WRITING\n\tPATH: %s\n\tSIZE: %d\n\tOFFSET: %d\n", p->filePath, p->size, p->dataOffset);
		fwrite(p, 1, sizeof(FileHeader), fp1);
	}

	char *s;
	directories.moveCursor(HEAD);
	while (s = directories.stepForward()) {
		printf("WRITING\n\tDIRECTORY: %s\n", s);
		fwrite(s, 1, 1024, fp1);
	}

	headers.moveCursor(HEAD);
	while (p = headers.stepForward()) {
		sprintf(completePath, "%s%s", this->root, p->filePath);
		FILE* fp2 = fopen(completePath, "rb");

		if (!fp2) {
			char c = 0;
			for (int i = 0; i < p->size; i++) {
				fwrite(&c, 1, 1, fp1);
			}
			continue;
		}

		unsigned char* buffer = new unsigned char[p->size];
		fread(buffer, 1, p->size, fp2);
		fwrite(buffer, 1, p->size, fp1);

		fclose(fp2);
		delete buffer;
	}

	fclose(fp1);

	return true;
}

bool FolderFile::compressDirectory(char* folder, char* destination) {
		
	LList<char*>* directories = new LList<char*>();
	LList<char*>* files = new LList<char*>();

	if (!decompressed) {
		ls_m(this->root, folder, files, directories);
		char* p;

		while (p = files->stepForward()) {
			printf("FILE: %s\n", p);
			addFile(p);
		}

		while (p = directories->stepForward()) {
			printf("DIR: %s\n", p);
			addFolder(p);
		}

		unify(destination);
		
		compressed = true;

		return true;
	}

	return false;
}

bool FolderFile::decompressDirectory(char* path, char* destination) {
	FileHeader* p;
	char completePath[128];

	if (!compressed) {
		FILE* fp1 = fopen(path, "rb");

		if (!mkdir_m(destination) &&  GetLastError() != ERROR_ALREADY_EXISTS) {
			printf("Can't create directory %s! (ERROR: %d)\n", destination, errno);
			return false;
		}

		if (!fp1)
			return false;

		char sig[4];
		int nBytes;
		int nFiles;
		int nDirectories;

		//Read in FolderFile header.
		fread(sig, 1, 4, fp1);
		fread(&nFiles, sizeof(unsigned int), 1, fp1);
		fread(&nDirectories, sizeof(unsigned int), 1, fp1);

		int count = 0;
			
		//Read in file headers
		while (count < nFiles) {
			p = new FileHeader();
			nBytes = fread(p, 1, sizeof(FileHeader), fp1);
			headers.addToEnd(p);
			count++;
		}

		count = 0;

		//Read in directories that need to be created
		while(count < nDirectories) {
			char* s = new char[1024];
			nBytes = fread(s, 1, 1024, fp1);
			s[nBytes] = 0;
			directories.addToEnd(s);
			count++;
		}

		//Make directories
		char* s;
		while (s = directories.stepForward()) {
			char filename[1024];
			sprintf(filename, "%s%s", destination, s);
			if (!mkdir_m(filename) &&  GetLastError() != ERROR_ALREADY_EXISTS) {
				printf("Can't create directory %s! (ERROR: %d)\n", filename, errno);
			}
		}

		//Write files
		while (p = headers.stepForward()) {
			unsigned char* buffer = new unsigned char[p->size];
			char filename[128];
			sprintf(filename, "%s%s", destination, p->filePath);

			fread(buffer, 1, p->size, fp1);
			FILE* fp2 = fopen(filename, "wb");
			if (fp2) {
				fwrite(buffer, 1, p->size, fp2);
				fclose(fp2);
			}

			delete buffer;
		}
		fclose(fp1);

		decompressed = true;

		return true;
	}

	return false;
}

int main(int argc, char* argv[]) {
	int token = 1;

	if (token) {
		FolderFile ff;
		ff.decompressDirectory("E:\\test\\compiled.ff", "E:\\test\\unzipped");
	}
	else {
		FolderFile ff("E:\\test");
		ff.compressDirectory("\\", "E:\\test\\compiled.ff");
	}
	
	return 0;
}