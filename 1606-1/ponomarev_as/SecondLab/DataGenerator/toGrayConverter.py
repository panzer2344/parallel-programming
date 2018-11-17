from PIL import Image, ImageFilter, ImageDraw
import sys
import random
import traceback
import os

def convert(image):
	return image.convert("L")
	
def load(filename):
	return Image.open(filename)
	
def data_to_file(image, filename):
	width, height = image.size
	output_file = open(filename, "w")
	
	output_file.write(str(width) + "\n" + str(height) + "\n")
	for y in range(height):
		for x in range(width):
			output_file.write(str(image.getpixel((x, y))))
			if x < width - 1:
				output_file.write(" ")
		output_file.write("\n")	

def file_to_image(filename):
	input_file = open(filename, "r")
	inWidth = inHeight = 0
	data = []
	image = []
	
	with open(filename, "r") as file:
		for line in file:
			data.append(line)
	
	inWidth = (int)(data.pop(0))
	inHeight = (int)(data.pop(0))
	
	resultImage = Image.new("L", (inWidth, inHeight))
	
	for y in range(inHeight):
		splitRow = data.pop(0).split(' ')
		image.append([])
		for x in range(inWidth):
			image[y].append((int)(splitRow[x]))
			resultImage.putpixel((x, y), (int)(splitRow[x]))
	return resultImage
	
def add_noise(factor, image):
	pix = image.load()
	width, height = image.size
	for i in range(width):
		for j in range(height):
			rand = random.randint(-factor, factor)
			a = pix[i, j][0] + rand
			b = pix[i, j][1] + rand
			c = pix[i, j][2] + rand
			if (a < 0):
				a = 0
			if (b < 0):
				b = 0
			if (c < 0):
				c = 0
			if (a > 255):
				a = 255
			if (b > 255):
				b = 255
			if (c > 255):
				c = 255
			image.putpixel((i, j), (a, b, c))
	return image

def print_exc_info(e):
	exc_type, exc_value, exc_traceback = sys.exc_info()
	# Extract unformatter stack traces as tuples
	trace_back = traceback.extract_tb(exc_traceback)
	# Format stacktrace
	stack_trace = list()
	for trace in trace_back:
		stack_trace.append("File : %s , Line : %d, Func.Name : %s, Message : %s" % (trace[0], trace[1], trace[2], trace[3]))
	
	print("Exception: " + str(e))
	print("Exception type: " + str(exc_type))
	print("Exception message: " + str(exc_value))
	print("Exception traceback: " + str(exc_traceback))
	print("Stack trace: " + str(stack_trace))
			
def help():		
	print("Use: scriptname option filename1 [filename2]")
	print("option could be --gen or --rns")
	print("--gen to generate data, needed 2 filenames")
	print("--rns to show and check data, needed 1 filename")

def main():
	try:
		#gen = generate
		if sys.argv[1] == "--gen":
			image = load(sys.argv[2])
			noiseImage = add_noise(50, image)
			grayImage = convert(noiseImage)
			grayImage.show()
			data_to_file(grayImage, sys.argv[3])
			
			image.close()
			noiseImage.close()
			grayImage.close()
			
		#rns = read and show
		if sys.argv[1] == "--rns":
			fileImage = file_to_image(sys.argv[2])
			fileImage.show()
			fileImage.close()
	except BaseException as e:
		print_exc_info(e)
		help()
		return
		
	os.system("pause")
	
	
if __name__ == "__main__":
	main()