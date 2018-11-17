#define _CRT_SECURE_NO_WARNINGS

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
	if (argc != 3) {
		std::cout << "count of arguments != 3" << std::endl;
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

int convolutionV(int* imageV, int width, int height, int index, int radius, int* smoothKernel, double smoothAlfa) {
	int resultValue = 0;
	
	for (int offsetY = -radius + 1; offsetY <= radius - 1; offsetY++) {
		for (int offsetX = -radius + 1; offsetX <= radius - 1; offsetX++) {
			
			int currentIndex = index + offsetY * width + offsetX;
			int currentRow = index / width;
			int currentColumn = index % width;
			
			if (currentRow + offsetY >= 0 && currentRow + offsetY < height && currentColumn + offsetX >= 0 && currentColumn + offsetX < width) {
				resultValue += imageV[currentIndex] * smoothKernel[(offsetY + radius - 1) * (2 * radius - 1) + (offsetX + radius - 1)];
			}
		}
	}
	return (int)((double)resultValue * smoothAlfa);
}

void smoothImageV(int* imageV, int width, int height) {
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
			int index = i * width + j;
			imageV[index] = convolutionV(imageV, width, height, index, radius, smoothKernel, smoothAlfa);
		}
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
	printSizes(width, height); //print sizes

	image = createImageMatrix(width, height); //creating image matrix
	loadImageFromFile(file, image, width, height); //load image from dataFile

	printFirstRow(image, width); // print first row of image
	printFirstColumn(image, height); // print first column of image

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


int main(int argc, char* argv[]) {
	int size = -1, rank = -1;
	int width = 0, height = 0;
	int** image = NULL;
	int* vecImage = NULL;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		//calculate size of pieces

		argcCheck(argc);
		std::cout << argv[1] << std::endl; //print dataFile name to checking  for correctness

		readImageFromFile(image, width, height, argv[1]); // read image from file
		vecImage = rePack(image, width, height);

		smoothImageV(vecImage, width, height); // smoothing

		writeImageIntoFileV(vecImage, width, height, argv[2]); //write image into file

		deleteImageMatrix(image, width, height); // delete image
		delete[] vecImage;

		system("pause");
	}

	MPI_Finalize();

	return 0;
}