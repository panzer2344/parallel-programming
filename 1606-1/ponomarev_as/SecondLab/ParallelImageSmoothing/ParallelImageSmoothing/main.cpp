#define _CRT_SECURE_NO_WARNINGS
//#define _DEBUG_
//#define _MINI_DEBUG_
#define _VERSIONS_COMPARE_

#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <ctime>
#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>

std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::stringstream tokenStream(s);

	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}

	return tokens;
}

int openDataFile(std::ifstream& file, const char* filename) {
	file.open(filename, std::ios::in);
	if (!file.is_open()) {
		std::cout << "Can't open input file" << std::endl;
		exit(-1);
	}
}

void argcCheck(int argc) {
	if (argc != 5) {
		std::cout << "count of arguments != 5" << std::endl;
		exit(-1);
	}
}

void getSize(int& size, std::ifstream& file) {
	std::string line;

	getline(file, line);
	size = std::stoi(line);
}

void getSizes(int& width, int& height, std::ifstream& file) {
	getSize(width, file);
	getSize(height, file);
}

void printSizes(int width, int height) {
	std::cout << width << std::endl;
	std::cout << height << std::endl;
}

int** createImageMatrix(int width, int height) {
	int** image = new int*[height];

	for (int i = 0; i < height; i++) {
		image[i] = new int[width];
	}

	return image;
}

void deleteImageMatrix(int** image, int width, int height) {
	for (int i = 0; i < height; i++) {
		delete[] image[i];
	}
	delete[] image;
}

void loadImageFromFile(std::ifstream& file, int** image, int width, int height) {
	std::string line;

	int i = 0;
	while (getline(file, line) && i < height) {
		std::vector<std::string> stringPixels = split(line, ' ');
		for (int j = 0; j < width; j++) {
			image[i][j] = std::stoi(stringPixels[j]);
		}
		i++;
	}
}

void printFirstRow(int** image, int width) {
	for (int i = 0; i < width; i++) {
		std::cout << image[0][i];
		if (i != width - 1)
			std::cout << ", ";
	}
	std::cout << std::endl;
}

void printFirstColumn(int** image, int height) {
	for (int j = 0; j < height; j++) {
		std::cout << image[j][0];
		if (j != height - 1)
			std::cout << ", ";
	}
	std::cout << std::endl;
}


int convolution(int** image, int width, int height, int i, int j, int radius, int* smoothKernel, double smoothAlfa) {
	int resultValue = 0;
	for (int offsetY = -radius + 1; offsetY <= radius - 1; offsetY++) {
		for (int offsetX = -radius + 1; offsetX <= radius - 1; offsetX++) {
			if (i + offsetY >= 0 && i + offsetY < height && j + offsetX >= 0 && j + offsetX < width) {
				resultValue += image[i + offsetY][j + offsetX] * smoothKernel[(offsetY + radius - 1) * (2 * radius - 1) + (offsetX + radius - 1)];
			}
		}
	}
	return (int)((double)resultValue * smoothAlfa);
}

void smoothImage(int** image, int width, int height) {
	/*int radius = 2;
	int smoothKernel[3][3] = {
	{ 1, 1, 1},
	{ 1, 1, 1},
	{ 1 ,1, 1},
	};
	double smoothAlfa = 1.0 / 9.0;*/

	int radius = 3;
	int smoothKernel[] = {
		1, 2, 3, 2, 1,
		2, 4, 6, 4, 2,
		3, 6, 9, 6, 3,
		2, 4, 6, 4, 2,
		1, 2, 3, 2, 1
	};
	double smoothAlfa = 1.0 / 81.0;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			image[i][j] = convolution(image, width, height, i, j, radius, smoothKernel, smoothAlfa);
		}
	}
}

int convolutionV(int* imageV, int width, int height, int index, int imageVSize, int radius, int* smoothKernel, double smoothAlfa) {
	int resultValue = 0;
	double _smoothAlfa = 0.0;
	
	for (int offsetY = -radius + 1; offsetY <= radius - 1; offsetY++) {
		for (int offsetX = -radius + 1; offsetX <= radius - 1; offsetX++) {
			
			int currentIndex = index + offsetY * width + offsetX;
			int currentRow = index / width;
			int currentColumn = index % width;
			int imSize = width * height;
			
			//if (currentRow + offsetY >= 0 && currentRow + offsetY < height && currentColumn + offsetX >= 0 && currentColumn + offsetX < width) {
			if(currentIndex >= 0 && currentIndex < imageVSize && currentColumn + offsetX >= 0 && currentColumn + offsetX < width){
				int smoothKernelCoeff = smoothKernel[(offsetY + radius - 1) * (2 * radius - 1) + (offsetX + radius - 1)];
				resultValue += imageV[currentIndex] * smoothKernelCoeff;
				_smoothAlfa += smoothKernelCoeff;
			}
		}
	}
	return (int)((double)resultValue / _smoothAlfa);
}

void smoothImageV(int* imageV, int imageVsize, int width, int height, int offset = 0, int realLen = 0) {
	/*int radius = 2;
	int smoothKernel[3][3] = {
	{ 1, 1, 1},
	{ 1, 1, 1},
	{ 1 ,1, 1},
	};
	double smoothAlfa = 1.0 / 9.0;*/

	/* init varriables */
	int radius = 3;
	int smoothKernel[] = {
		1, 2, 3, 2, 1,
		2, 4, 6, 4, 2,
		3, 6, 9, 6, 3,
		2, 4, 6, 4, 2,
		1, 2, 3, 2, 1
	};
	double smoothAlfa = 1.0 / 81.0;

	//int imSize = width * height;

	//int offsetY = offset / width;
	//int offsetX = offset % width;

	if (realLen == 0) {
		realLen = width * height;
	}

	//for (int i = offsetY; i < height; i++) {
	//	for (int j = offsetX; j < width; j++) {
	//		int index = i * width + j;
	//		imageV[index] = convolutionV(imageV, width, height, index, radius, smoothKernel, smoothAlfa);
	//	}
	//}

	int maxIndex = offset + realLen;

	for (int index = offset; index < maxIndex; index++) {
		imageV[index] = convolutionV(imageV, width, height, index, imageVsize, radius, smoothKernel, smoothAlfa);
		//imageV[index] = (imageV[index - 1] + imageV[index]) / 2;
	}
}

void writeImageIntoFile(int** image, int width, int height, const char* filename) {
	std::ofstream outputFile(filename);

	outputFile << width << std::endl;
	outputFile << height << std::endl;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			outputFile << image[i][j] << " ";
		}
		outputFile << std::endl;
	}

	outputFile.close();
}

void writeImageIntoFileV(int* imageV, int width, int height, const char* filename) {
	std::ofstream outputFile(filename);

	outputFile << width << std::endl;
	outputFile << height << std::endl;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			outputFile << imageV[i * width + j] << " ";
		}
		outputFile << std::endl;
	}

	outputFile.close();
}

void readImageFromFile(int**& image, int& width, int& height, const char* filename) {
	std::ifstream file;

	openDataFile(file, filename); //open dataFile

	getSizes(width, height, file); // get width and height

#ifdef _DEBUG_
	printSizes(width, height); //print sizes
#endif //_DEBUG_

	image = createImageMatrix(width, height); //creating image matrix
	loadImageFromFile(file, image, width, height); //load image from dataFile

#ifdef _DEBUG_
	printFirstRow(image, width); // print first row of image
	printFirstColumn(image, height); // print first column of image
#endif // _DEBUG_

	file.close();
}

int* rePack(int** image, int width, int height) {
	int* vector = new int[width * height];

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			vector[i * width + j] = image[i][j];
		}
	}

	return vector;
}

void calculateSizes(int procCount, int width, int height, int* offsets, int* offsetsL, int* offsetsR, int* realLens, int* sendLens, int smoothKernelRadius) {
	int imSize = width * height;
	int startLen = imSize / procCount;
	int tail = imSize % procCount;

	offsets[0] = 0;
	realLens[0] = startLen;
	sendLens[0] = startLen + (offsets[0] + startLen + (smoothKernelRadius - 1) * width > imSize ? startLen + tail : (smoothKernelRadius - 1) * (width + 1)); // sendLen + bottom border for convolution
	offsetsL[0] = 0;
	offsetsR[0] = offsetsL[0] + sendLens[0] - 1; // offsetsR points to last send element


	for (int i = 1; i < procCount; i++) {
		offsets[i] = offsets[i - 1] + realLens[i - 1]; 
		realLens[i] = i == procCount - 1 ? startLen + tail : startLen; // last process will calculate more. its better to make this tail for first(host) process, but i mb will do this in near future)
		
		int leftOffset = offsets[i] - (smoothKernelRadius - 1) * width - ( ( offsets[i] % width != 0 ) ? smoothKernelRadius - 1 : 0); // the last minus is needed if sent data isnt placed at the top of the line
		offsetsL[i] = (leftOffset > 0) ? leftOffset : 0; // check if offset move out from array bounds

		int rightOffset = offsets[i] + realLens[i] + (smoothKernelRadius - 1) * width + (((offsets[i] + realLens[i]) % width != 0) ? smoothKernelRadius - 1 : 0); // similary with last plus
		offsetsR[i] = (rightOffset - 1 < imSize - 1) ? rightOffset - 1 : imSize - 1; // check if offset move out from array bounds

		sendLens[i] = offsetsR[i] - offsetsL[i] + 1;
	}
}

void rvs_calculateSizes(int procCount, int width, int height, int* offsets, int* offsetsL, int* offsetsR, int* realLens, int* sendLens, int smoothKernelRadius) {
	int imSize = width * height;
	int startLen = imSize / procCount;
	int tail = imSize % procCount;

	offsets[procCount - 1] = 0;
	realLens[procCount - 1] = startLen;
	sendLens[procCount - 1] = startLen + (offsets[procCount - 1] + startLen + width > imSize ? startLen + tail : width); // sendLen + bottom border for convolution
	offsetsL[procCount - 1] = 0;
	offsetsR[procCount - 1] = offsetsL[procCount - 1] + sendLens[procCount - 1] - 1; // offsetsR points to last send element


	for (int i = procCount - 2; i > 0; i--) {
		offsets[i] = offsets[i + 1] + realLens[i + 1];
		realLens[i] = i == procCount - 1 ? startLen + tail : startLen; // last process will calculate more. its better to make this tail for first(host) process, but i mb will do this in near future)

		int leftOffset = offsets[i] - (smoothKernelRadius - 1) * width - ((offsets[i] % width != 0) ? smoothKernelRadius - 1 : 0); // the last minus is needed if sent data isnt placed at the top of the line
		offsetsL[i] = (leftOffset > 0) ? leftOffset : 0; // check if offset move out from array bounds

		int rightOffset = offsets[i] + realLens[i] + (smoothKernelRadius - 1) * width + (((offsets[i] + realLens[i]) % width != 0) ? smoothKernelRadius - 1 : 0); // similary with last plus
		offsetsR[i] = (rightOffset < imSize - 1) ? rightOffset : imSize - 1; // check if offset move out from array bounds

		sendLens[i] = offsetsR[i] - offsetsL[i] + 1;
	}
}

// my ScatterV
int* MY_ScattervINT(const int *sendbuf, const int *sendcounts, const int *offsetsL, const int* realOffsets, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, int procCount, int rank) {
	/* Another crutch. Another one repack, which will slow down all, but without it we cant do anything(mpi throws exceptions about array length). We will return new array to delete it in end of the program */
	
	// define vars for crutch
	int newSendBufSize = 0;
	int* newSendBuf = NULL;
	int* newOffsets = new int[procCount];

	// init crutch vars
	if (rank == root) {
		for (int i = 0; i < procCount; i++) {
			newSendBufSize += sendcounts[i];
		}
		newSendBuf = new int[newSendBufSize];
		newOffsets[0] = 0;
		for (int i = 0; i < procCount; i++) {
			for (int j = 0; j < sendcounts[i]; j++) {
				newSendBuf[newOffsets[i] + j] = sendbuf[offsetsL[i] + j];
			}
			if (i + 1 < procCount) {
				newOffsets[i + 1] = offsetsL[i] + sendcounts[i] - 1;
			}
		}
	}
	MPI_Bcast(newOffsets, procCount, MPI_INT, root, MPI_COMM_WORLD);

	if (rank == root) {
		for (int i = 0; i < procCount; i++) {
			if (i != root) {
#ifdef _DEBUG_
				std::cout << "Start sending to " << i << std::endl;
				std::cout << "SendBuf adress: " << std::hex << sendbuf << std::endl;
				std::cout << "SendBuf + displs[i] adress: " << std::hex << sendbuf + offsetsL[i] << std::endl;
				std::cout << "Offset = " << std::dec << offsetsL[i] << std::endl;
				std::cout << "SendCount = " << sendcounts[i] << std::endl;
				system("pause");
#endif // _DEBUG_
				MPI_Send(newSendBuf + newOffsets[i], sendcounts[i], sendtype, i, 0, MPI_COMM_WORLD);
#ifdef _DEBUG_
				std::cout << "End sending to " << i << std::endl;
				system("pause");
#endif //_DEBUG_
			}
		}
	}
	else {
		MPI_Status status;
#ifdef _DEBUG_
		std::cout << "Start recieving from root in " << rank << "process" << std::endl;
#endif // _DEBUG_
		MPI_Recv(recvbuf, recvcount, recvtype, root, 0, MPI_COMM_WORLD, &status);
#ifdef _DEBUG_
		std::cout << "End recieving from root in " << rank << "process" << std::endl;
#endif // _DEBUG_
	}

	MPI_Barrier(MPI_COMM_WORLD);
	delete[] newOffsets;

	return newSendBuf;
}

void fromSendToRecvVec(int* sendVec, int* recvVec, int offsetReal, int offsetL, int realLen) {
	int dif = offsetReal - offsetL;
	for(int index = dif; index < dif + realLen; index++){
		recvVec[index - dif] = sendVec[index];
	}
}

void writeSendedPiecesIntoFile(std::string filename, int* imVec, int width, int imSize, int* offsetsL, int* offsetsR, int* offsetsReal, int* sendedLens, int procCount) {
	std::ofstream outFile(filename);

	std::cout << "Enter the write debug file" << std::endl;

	if (!outFile.is_open()) {
		std::cout << "Cant open debug file" << std::endl;
	}

	outFile << "Image size = " << imSize << std::endl;
	outFile << "procCount = " << procCount << std::endl;
	outFile << "Image width = " << width << std::endl;
	outFile << std::endl << std::endl << std::endl;

	for (int proc = 0; proc < procCount; proc++) {
		outFile << "For proc number " << proc << std::endl;
		outFile << "OffsetL = " << offsetsL[proc] << std::endl;
		outFile << "OffsetR = " << offsetsR[proc] << std::endl;
		outFile << "OffsetReal = " << offsetsReal[proc] << std::endl;
		outFile << "Sended len = " << sendedLens[proc] << std::endl;
		outFile << std::endl;

		for (int index = offsetsL[proc]; index < offsetsL[proc] + sendedLens[proc]; index++) {
			outFile << imVec[index] << " ";
			if (index % width == width - 1)
				outFile << std::endl;
		}

		outFile << std::endl << std::endl;
	}

}


int main(int argc, char* argv[]) {
	int size = -1, rank = -1;
	int width = 0, height = 0;
	int** image = NULL;
	int* vecImage = NULL;
	int imageSize = 0;

	int smoothKernelRadius = 3; // its bad way, that this varriable doesnt influence to same var in smoothImage function, but but let it be so far

	int* offsets; // an array of offsets to the pixels that each process will have to process
	int* offsetsL; // and array of offsets to left border of pixels which will be needed for processing
	int* offsetsR; // and array of offsets to right border of pixels which will be needed for processing
	int* realLens; // an array of lengthes of pixel arrays that each process will have to process
	int* sendedLens; // and array of lengthes of pixel arrays which will be needed for processing

	int* pieceOfVecImgSended;
	int sizeOfMaxPieceOfVecImgSended = 0;

	int* pieceOfVecImgRecieved;
	int sizeOfMaxPieceOfVecImgRecieved = 0;

	int* newVecImage = NULL; // array needed for crutch

	//std::string filenameResults("D:\\Git\\MPI_Projects\\ImageSmoothing\\Data\\results.txt");
	std::string filenameResults(argv[4]);
	double startWTimeLinear = 0.0;
	double endWTimeLinear = 0.0;
	double startWTimeParallel = 0.0;
	double endWTimeParallel = 0.0;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


	/* Linear version */
#ifdef _VERSIONS_COMPARE_
	if (rank == 0) {
		std::cout << "Linear version" << std::endl;
		
		argcCheck(argc);

#ifdef _DEBUG_
		std::cout << argv[1] << std::endl; //print dataFile name to checking  for correctness
#endif //_DEBUG_

		readImageFromFile(image, width, height, argv[1]); // read image from file

		startWTimeLinear = MPI_Wtime();

		smoothImage(image, width, height); // smooth

		endWTimeLinear = MPI_Wtime();

		writeImageIntoFile(image, width, height, argv[3]); //write image into file

		deleteImageMatrix(image, width, height); // delete image

		//system("pause");
	}
#endif // _VERSIONS_COMPARE_

	/* Parallel version */
	
	/* creating arrays of lengthes and offsets */
	offsets = new int[size];
	offsetsL = new int[size];
	offsetsR = new int[size];
	realLens = new int[size];
	sendedLens = new int[size];

	if (rank == 0) {
		std::cout << "Parallel version" << std::endl;

		argcCheck(argc);

#ifdef _DEBUG_
		std::cout << argv[1] << std::endl; //print dataFile name to checking  for correctness
#endif // _DEBUG_

		readImageFromFile(image, width, height, argv[1]); // read image from file

		startWTimeParallel = MPI_Wtime();

		vecImage = rePack(image, width, height); // repack image array from matrix to vector form

		//calculating offsets and real and sended lens
		calculateSizes(size, width, height, offsets, offsetsL, offsetsR, realLens, sendedLens, smoothKernelRadius); // calculating all needed values for using smooth function for each pieces in each process

		//mini - debugging
#ifdef _MINI_DEBUG_
		std::cout << "Size: " << size << std::endl;
		std::cout << "Width: " << width << std::endl;
		std::cout << "Height: " << height << std::endl;

		std::cout << "Offsets: [ ";
		for (int i = 0; i < size; i++) {
			std::cout << offsets[i] << (i < size - 1 ? ", " : " ");
		}
		std::cout << "]" << std::endl;
		system("pause");

		std::cout << "OffsetsL: [ ";
		for (int i = 0; i < size; i++) {
			std::cout << offsetsL[i] << (i < size - 1 ? ", " : " ");
		}
		std::cout << "]" << std::endl;
		system("pause");

		std::cout << "OffsetsR: [ ";
		for (int i = 0; i < size; i++) {
			std::cout << offsetsR[i] << (i < size - 1 ? ", " : " ");
		}
		std::cout << "]" << std::endl;
		system("pause");

		std::cout << "realLens: [ ";
		for (int i = 0; i < size; i++) {
			std::cout << realLens[i] << (i < size - 1 ? ", " : " ");
		}
		std::cout << "]" << std::endl;
		system("pause");

		std::cout << "sendedLens: [ ";
		for (int i = 0; i < size; i++) {
			std::cout << sendedLens[i] << (i < size - 1 ? ", " : " ");
		}
		std::cout << "]" << std::endl;
		system("pause");

		std::cout << "Kernel radius: " << smoothKernelRadius << std::endl;
		system("pause");
#endif // _MINI_DEBUG_
		//end of mini-debugging
	}

	// sending arrays of offsetts and lengthes to all process
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast offsets" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(offsets, size, MPI_INT, 0, MPI_COMM_WORLD);  // broadcasting offsets
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast offsets" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast offsetsL" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(offsetsL, size, MPI_INT, 0, MPI_COMM_WORLD); // broadcasting offsetsL
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast offsetsL" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast offsetsR" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(offsetsR, size, MPI_INT, 0, MPI_COMM_WORLD); // broadcasting offsetsR
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast offsetsR" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast realLens" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(realLens, size, MPI_INT, 0, MPI_COMM_WORLD); // broadcasting realLens
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast realLens" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast sendedLens" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(sendedLens, size, MPI_INT, 0, MPI_COMM_WORLD); // broadcasting sendedLens
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast sendedLens" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast width" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD); // broadcasting width
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast width" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of bcast height" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD); // broadcasting height
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of bcast height" << std::endl;
		system("pause");
	}
#endif //_DEBUG_


	// init needed vars
	imageSize = width * height;
	sizeOfMaxPieceOfVecImgSended = sendedLens[rank];
	pieceOfVecImgSended = new int[sizeOfMaxPieceOfVecImgSended];

	sizeOfMaxPieceOfVecImgRecieved = realLens[rank];
	pieceOfVecImgRecieved = new int[sizeOfMaxPieceOfVecImgRecieved];

#ifdef _DEBUG_
	std::cout << "ImageSize = " << imageSize << std::endl;
	std::cout << "SizeOfMaxPieceOfVecImgSended = " << sizeOfMaxPieceOfVecImgSended << std::endl;
	std::cout << "SizeOfMaxPieceOfVecImgRecieved = " << sizeOfMaxPieceOfVecImgRecieved << std::endl;
#endif // _DEBUG_
	
	//if (rank != 0) {
	//	vecImage = new int[imageSize];
	//}

	// sending pieces of image vector
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of scatterV" << std::endl;
		system("pause");
		system("pause");
	}
#endif //_DEBUG_
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Scatterv(vecImage, sendedLens, offsetsL, MPI_INT, pieceOfVecImgSended, sizeOfMaxPieceOfVecImgSended, MPI_INT, 0, MPI_COMM_WORLD); // scatter(send to all proc) of vecImage
	//newVecImage = MY_ScattervINT(vecImage, sendedLens, offsetsL, offsets, MPI_INT, pieceOfVecImg, sizeOfMaxPieceOfVecImg, MPI_INT, 0, size, rank);
	MPI_Barrier(MPI_COMM_WORLD);
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of ScatterV" << std::endl;
		system("pause");
		system("pause");
	}
#endif //_DEBUG_

	// write sended pieces into file
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "In writing debug file branch" << std::endl;
		std::string debugFile("D:\\Git\\MPI_Projects\\ImageSmoothing\\Data\\debugData.txt");
		writeSendedPiecesIntoFile(debugFile, vecImage, width, imageSize, offsetsL, offsetsR, offsets, sendedLens, size);
	}
#endif // _DEBUG_


// debug output first item of pieces
#ifdef _DEBUG_
	for (int i = 0; i < 5; i++) {
		std::cout << "In rank: " << rank << "pieceOfVecImg[" << pieceOfVecImgSended[i] << "]" << std::endl;
	}
	if (rank == 0)
		system("pause");
#endif //_DEBUG_

#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of smoothImageV" << std::endl;
		system("pause");
	}
#endif //DEBUG
	MPI_Barrier(MPI_COMM_WORLD);
	smoothImageV(pieceOfVecImgSended, sizeOfMaxPieceOfVecImgSended, width, height, offsets[rank] - offsetsL[rank], realLens[rank]); // smoothing
	MPI_Barrier(MPI_COMM_WORLD);
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of smoothImageV" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

	// repack from sending to recieving array
	MPI_Barrier(MPI_COMM_WORLD);
	fromSendToRecvVec(pieceOfVecImgSended, pieceOfVecImgRecieved, offsets[rank], offsetsL[rank], realLens[rank]);
	MPI_Barrier(MPI_COMM_WORLD);

	//sending result back to host process
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "start of gatherV" << std::endl;
		system("pause");
	}
#endif //_DEBUG_
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Gatherv(pieceOfVecImgRecieved, sizeOfMaxPieceOfVecImgRecieved, MPI_INT, vecImage, realLens, offsets, MPI_INT, 0, MPI_COMM_WORLD); // gather(recieve from all proc) in vecImage
	MPI_Barrier(MPI_COMM_WORLD);
#ifdef _DEBUG_
	if (rank == 0) {
		std::cout << "end of GatherV" << std::endl;
		system("pause");
	}
#endif //_DEBUG_

	//end time counting for parallel version
	if (rank == 0) {
		endWTimeParallel = MPI_Wtime();
	}

	// writing to file and delete image
	if(rank == 0){
#ifdef _DEBUG_
		std::cout << "start writing image into file" << std::endl;
#endif // _DEBUG_
		writeImageIntoFileV(vecImage, width, height, argv[2]); //write image into file
#ifdef _DEBUG_
		std::cout << "end writing image into file" << std::endl;
#endif // _DEBUG_

#ifdef _DEBUG_
		std::cout << "start deleting image matrix" << std::endl;
#endif // _DEBUG_
		deleteImageMatrix(image, width, height); // delete image
#ifdef _DEBUG_
		std::cout << "end deleting image matrix" << std::endl;
#endif // _DEBUG_

#ifdef _DEBUG_
		std::cout << "start deleting image vector" << std::endl;
#endif // _DEBUG_
		delete[] vecImage;
#ifdef _DEBUG_
		std::cout << "end deleting image vector" << std::endl;
#endif // _DEBUG_

		system("pause");
	}

	// write result times to the file
	if (rank == 0) {
		std::ofstream resFile(filenameResults, std::ios::app);

		time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::_Timeobj<char, const tm*> nowtime = std::put_time(localtime(&now), "%F %T");

		resFile << "Results( "<< nowtime << "): " << std::endl << std::endl << std::endl;
		resFile << "Linear time = " << endWTimeLinear - startWTimeLinear << std::endl << std::endl;
		resFile << "Parallel time = " << endWTimeParallel - startWTimeParallel << std::endl << std::endl;

		std::cout << "Results( " << nowtime << "): " << std::endl << std::endl << std::endl;
		std::cout << "Linear time = " << endWTimeLinear - startWTimeLinear << std::endl << std::endl;
		std::cout << "Parallel time = " << endWTimeParallel - startWTimeParallel << std::endl << std::endl;

		resFile.close();
	}

	MPI_Finalize();

	return 0;
}