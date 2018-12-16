#include <mpi.h>
#include <opencv\cv.hpp>
#include <iostream>


using namespace std;
using namespace cv;



//int convolutionV(uchar* imageV, int width, int height, int index, int imageVSize, int radius, int* kernelR, int* kernelC) {
//
//	int kernelSize = 2 * radius - 1;
//	int result = 0.0;
//
//
//	for (int i = 0; i < kernelSize; i++) {
//		
//		imageV
//	
//	}
//
//}


int convolutionV(uchar* imageV, int width, int height, int index, int imageVSize, int radius, char* kernel) {
	int resultValue = 0;

	for (int offsetY = -radius + 1; offsetY <= radius - 1; offsetY++) {
		for (int offsetX = -radius + 1; offsetX <= radius - 1; offsetX++) {

			int currentIndex = index + offsetY * width + offsetX;
			int currentRow = index / width;
			int currentColumn = index % width;
			int imSize = width * height;

			//if (currentRow + offsetY >= 0 && currentRow + offsetY < height && currentColumn + offsetX >= 0 && currentColumn + offsetX < width) {
			if (currentIndex >= 0 && currentIndex < imageVSize){// && currentColumn + offsetX >= 0 && currentColumn + offsetX < width) {
				int kernelCoeff = kernel[(offsetY + radius - 1) * (2 * radius - 1) + (offsetX + radius - 1)];
				resultValue += imageV[currentIndex] * kernelCoeff;
			}
		}
	}
	return resultValue;
}

int bordered(int value, int min , int max) {

	if (value < min) {
		return min;
	}
	else if (value > max) {
		return max;
	}
	else {
		return value;
	}

}

void mySobel(uchar* imageV, int imageVsize, int width, int height, int offset = 0, int realLen = 0, int rank = 0) {

	/* init varriables */
	int radius = 2;
	/*int grad_yKernel[] = {
		 -1, -2, -1,
		 0, 0, 0,
		 1, 2, 1,
	};

	int grad_xKernel[] = {
		-1, 0, 1,
		-2, 0, 2,
		-1, 0, 1,
	};*/


	char grad_yKernel[] = {
		-1, -2, -1,
		0, 0, 0,
		1, 2, 1,
	};

	char grad_xKernel[] = {
		-1, 0, 1,
		-2, 0, 2,
		-1, 0, 1,
	};

	if (realLen == 0) {
		realLen = width * height;
	}

	int maxIndex = offset + realLen;

	uchar* imageV1 = new uchar[imageVsize];

	for (int i = 0; i < imageVsize; i++) {
	
		imageV1[i] = imageV[i];

	}

	//cout << "[ " << (int)imageV[0] << ", " << offset << ", " << maxIndex << " ]" << endl;

	for (int index = offset; index < maxIndex; index++) {

		int grad_x = bordered(convolutionV(imageV1, width, height, index, imageVsize, radius, grad_xKernel), 0, 255);
		int grad_y = bordered(convolutionV(imageV1, width, height, index, imageVsize, radius, grad_yKernel), 0, 255);

		/*if (index == 123 * width) {
		
			
			cout << "in rank#" << rank << "[" << (int)imageV1[index - width - 1] << ", " << (int)imageV1[index - width] << ", " << (int)imageV1[index - width + 1] << "]" << endl;
			cout << "in rank#" << rank << "[" << (int)imageV1[index - 1] << ", " << (int)imageV1[index] << ", " << (int)imageV1[index + 1] << "]" << endl;
			cout << "in rank#" << rank << "[" << (int)imageV1[index + width - 1] << ", " << (int)imageV1[index + width] << ", " << (int)imageV1[index + width + 1] << "]" << endl;
		
		}*/

		imageV[index] =  (uchar)sqrt( grad_x * grad_x + grad_y * grad_y );
		//imageV[index] = (uchar)(0.5 * abs(grad_x) + 0.5 * abs(grad_y));
	}
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

		int leftOffset = offsets[i] - (smoothKernelRadius - 1) * width - ((offsets[i] % width != 0) ? smoothKernelRadius - 1 : 0); // the last minus is needed if sent data isnt placed at the top of the line
		offsetsL[i] = (leftOffset > 0) ? leftOffset : 0; // check if offset move out from array bounds

		int rightOffset = offsets[i] + realLens[i] + (smoothKernelRadius - 1) * width + (((offsets[i] + realLens[i]) % width != 0) ? smoothKernelRadius - 1 : 0); // similary with last plus
		offsetsR[i] = (rightOffset - 1 < imSize - 1) ? rightOffset - 1 : imSize - 1; // check if offset move out from array bounds

		sendLens[i] = offsetsR[i] - offsetsL[i] + 1;
	}

}

void fromSendToRecvVec(uchar* sendVec, uchar* recvVec, int offsetReal, int offsetL, int realLen) {
	//cout << "start fromSendToRecvVec" << endl;
	
	int dif = offsetReal - offsetL;
	for (int index = dif; index < dif + realLen; index++) {
		recvVec[index - dif] = sendVec[index];
	}

	//cout << "end fromSendToRecvVec" << endl;
}


Mat initImage(Mat& image) {

	
	Mat src_gray, grad;
	int scale = 1, delta = 0, ddepth = CV_16S;

	//cout << image.cols << endl;

	GaussianBlur(image, src_gray, Size(3, 3), 0, 0, BORDER_DEFAULT);

	//cout << "try to cvtColor1" << endl;

	/// Convert it to gray
	cvtColor(src_gray, src_gray, CV_BGR2GRAY);
	src_gray.convertTo(src_gray, CV_8UC1);

	//cout << "color converted" << endl;

	return src_gray;

}

void show(Mat& src_gray, double time, string& type) {

	string window_name = type;
	window_name.append(" version ");
	window_name.append(to_string(time));
	window_name.append("ms");

	/// Create window
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	imshow(window_name, src_gray);

	cout << type << " time is " << time << endl;

	waitKey();

}


Mat linearVersion(int rank, Mat& image) {


	double startWTimeLinear = 0.0;
	Mat src_gray;


	if (rank == 0) {

		//cout << "start initImage2" << endl;
		src_gray = initImage(image);
		//cout << "end initImage2" << endl;

		//cout << src_gray;


		startWTimeLinear = MPI_Wtime();
		//mySobel
		mySobel(src_gray.data, src_gray.rows * src_gray.cols, src_gray.cols, src_gray.rows);

		string type("Linear");
		show(src_gray, MPI_Wtime() - startWTimeLinear, type);


	}

	return src_gray;

}


Mat parallelVersion(int rank, int size, Mat& image) {

	int* sendedLens = new int[size]; // and array of lengthes of pixel arrays which will be needed for processing
	int* offsetsL = new int[size]; // and array of offsets to left border of pixels which will be needed for processing
	int* offsets = new int[size]; // an array of offsets to the pixels that each process will have to process
	int* offsetsR = new int[size]; // and array of offsets to right border of pixels which will be needed for processing
	int* realLens = new int[size];

	Mat src_gray;

	uchar* pieceOfVecImgSended;
	int sizeOfMaxPieceOfVecImgSended = 0;

	uchar* pieceOfVecImgRecieved;
	int sizeOfMaxPieceOfVecImgRecieved = 0;

	int* newVecImage = NULL; // array needed for crutch
	uchar* vecImage = NULL;


	double startWTimeParallel = 0.0;

	int smoothKernelSize = 3;

	int offset = 0, offsetR = 0, realLen = 0;

	int width, height;



	if (rank == 0) {

		//int* offsets = new int[size]; // an array of offsets to the pixels that each process will have to process
		//int* offsetsR = new int[size]; // and array of offsets to right border of pixels which will be needed for processing
		//int* realLens = new int[size]; // an array of lengthes of pixel arrays that each process will have to process


		//cout << "start initImage" << endl;
		src_gray = initImage(image);
		//cout << "end initImage" << endl;


		width = src_gray.cols;
		height = src_gray.rows;

		startWTimeParallel = MPI_Wtime(); // start counting down time

		calculateSizes(size, src_gray.cols, src_gray.rows, offsets, offsetsL, offsetsR, realLens, sendedLens, smoothKernelSize);

		//cout << "sizes calculated" << endl;

		vecImage = src_gray.data;

		if (size == 1) {

			mySobel(vecImage, src_gray.cols * src_gray.rows, src_gray.cols, src_gray.rows); // smoothing
			
			string type("Parallel");
			show(src_gray, MPI_Wtime() - startWTimeParallel, type);
			
			return src_gray;

		}

		//cout << "vecImg[0] = " << (int)(*vecImage) << "vecImage[1]" << (int)(*(vecImage + 1)) << " vecImg[0] = " << (int)vecImage[0] << endl;


		//for (int i = 1; i < size; i++) {
		//
		//	cout << "start sending" << endl;

		//	MPI_Send(&offsets[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		//	//MPI_Send(&offsetsL[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		//	MPI_Send(&offsetsR[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		//	MPI_Send(&realLens[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		//	//MPI_Send(&sendedLens[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		//
		//	cout << i << " sended" << endl;

		//}

		offset = offsets[0];
		offsetR = offsetsR[0];
		realLen = realLens[0];

		//cout << "ofs 0 readed" << endl;

	}
	else {
	
		/*MPI_Status status;

		MPI_Recv(&offset, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(&offsetR, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(&realLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);*/

	}

	//cout << "bcast" << endl;

	MPI_Bcast(offsets, size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(offsetsL, size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(offsetsR, size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(sendedLens, size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(realLens, size, MPI_INT, 0, MPI_COMM_WORLD);


	MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);



	/*cout << "sl in #" << rank << " is " << sendedLens[rank] << endl;
	cout << "rl in #" << rank << " is " << realLens[rank] << endl;
	cout << "o in #" << rank << " is " << offsets[rank] << endl;
	cout << "ol in #" << rank << " is " << offsetsL[rank] << endl;
	cout << "or in #" << rank << " is " << offsetsR[rank] << endl;*/


	sizeOfMaxPieceOfVecImgSended = sendedLens[rank];
	pieceOfVecImgSended = new uchar[sizeOfMaxPieceOfVecImgSended];

	sizeOfMaxPieceOfVecImgRecieved = realLens[rank];
	pieceOfVecImgRecieved = new uchar[sizeOfMaxPieceOfVecImgRecieved];


	/*cout << sendedLens[rank] << ", " << realLen << endl;
	cout << sizeOfMaxPieceOfVecImgSended << ", " << sizeOfMaxPieceOfVecImgRecieved << endl;*/



	//i dont know why, but mpi say to me, that sizeOf...Sended = 0, more preciesely it said, that rcount = 0
	MPI_Scatterv(vecImage, sendedLens, offsetsL, MPI_UNSIGNED_CHAR, pieceOfVecImgSended, sizeOfMaxPieceOfVecImgSended, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD); // scatter(send to all proc) of vecImage

	/*cout << "Before Sobel " << (int)pieceOfVecImgSended[0] << endl;

	cout << "offset in #" << rank << " is " << offset << endl;
	cout << "offsetL in #" << rank << " is " << offsetsL[rank] << endl;*/

	//MPI_Barrier(MPI_COMM_WORLD);
	//cout << "mySobel in rank#" << rank << endl;
	mySobel(pieceOfVecImgSended, sizeOfMaxPieceOfVecImgSended, width, height, offsets[rank] - offsetsL[rank], realLens[rank], rank); // smoothing
	//MPI_Barrier(MPI_COMM_WORLD);

	//cout << "After Sobel " << (int)pieceOfVecImgSended[0] << endl;

	fromSendToRecvVec(pieceOfVecImgSended, pieceOfVecImgRecieved, offsets[rank], offsetsL[rank], realLens[rank]); // cutting vectors

	//cout << "After fromSendToRecvVec " << (int)pieceOfVecImgRecieved[0] << endl;

	MPI_Barrier(MPI_COMM_WORLD);

	/*cout << "#" << rank <<
	"povir = " << (int)pieceOfVecImgRecieved << " sompovir = " << sizeOfMaxPieceOfVecImgRecieved << " vi = " << (int)vecImage << " rl = " << (int)realLens << " os = " << (int)offsets <<
	"rl = " << (int)realLen << " offsets = " << (int)offset << endl;*/

	MPI_Gatherv(pieceOfVecImgRecieved, sizeOfMaxPieceOfVecImgRecieved, MPI_UNSIGNED_CHAR, vecImage, realLens, offsets, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD); // gather(recieve from all proc) in vecImage
	MPI_Barrier(MPI_COMM_WORLD);

	//cout << "end gather" << endl;


	if (rank == 0) {
	
		string type("Parallel");
		show(src_gray, MPI_Wtime() - startWTimeParallel, type);
	
	}

	return src_gray;


}

bool isEqual(Mat& m1, Mat& m2) {

	if (m1.cols != m2.cols || m1.rows != m2.rows) {
		return false;
	}

	for (int i = 0; i < m1.rows; i++) {
	
		for (int j = 0; j < m1.cols; j++) {
		
			if (m1.at<uchar>(i, j) != m2.at<uchar>(i, j)) {

				cout << "at (" << i << ", " << j << ") pix1 = " << (int)m1.at<uchar>(i, j) << " pix2 = " << (int)m2.at<uchar>(i, j) << endl;
				cout << "by pData: " <<  (int)(*(m1.data + m1.cols * i + j)) << endl;
				cout << "by pData: " <<  (int)(*(m2.data + m2.cols * i + j)) << endl;

				return false;
			}
		
		}
	
	}

	return true;

}




int main(int argc, char* argv[]) {

	int rank, size;
	Mat image;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (rank == 0) {

		if (argc != 2) {
			cout << "use <enviroment with arguments> ThirdLab.exe <filename>" << endl;
			return -1;
		}

		String filename(argv[1]);
	
		image = imread(filename);
		//image = imread("image.png");
		//image = imread("triangle.jpg");
		//image = imread("mini.png");
		//Mat image = imread("facebook.png");
		//image = imread("birds.jpg");
		//image = imread("miniMouse.jpg");

		if (image.cols == 0 || image.rows == 0) {
		
			cout << "cant load file " << argv[1] << endl;
			return -1;
		}

		//cout << image.cols << endl;
		//cout << image.rows << endl;

	}

	Mat lin = linearVersion(rank, image);

	Mat par = parallelVersion(rank, size, image);

	if (rank == 0) {

		cout << (isEqual(par, lin) ? "matrixes are equal" : "matrixes aren't equal") << endl;

	}

	MPI_Finalize();

}