import string
import os
import glob
import tkinter as tk
from tkinter import filedialog
from tkinter import messagebox
from PIL import Image
import shutil

sizes = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
jpg_ext = [".jpg", ".jpeg", ".JPEG", ".JPG"]
png_ext = [".png", ".PNG"]

def get_root_folder(root_tk):
	root_tk.withdraw()
	folder_path = filedialog.askdirectory(title='Choose textures folder...')
	if folder_path:
		return folder_path
	else:
		root_tk.withdraw()
		tk.messagebox.showerror("Error", "Folder not selected or invalid.")
		root_tk.destroy()
		exit()

# Using the solution to reduce image from: https://github.com/RyanAWalters/PowerOf2ImageResizer/blob/master/po2resizer.py
# I will use the threshold solution also
def get_closest_size(actual_size):
    return min(sizes, key=lambda size: abs(size - actual_size))

def get_best_size(img, threshold=0.25):
	x, y = img.size
	max_size = max(x, y)
	new_size = max(get_closest_size(x), get_closest_size(y))
	if new_size < max_size:
		if (max_size - new_size) > int(new_size * threshold):	# if it's going to reduce more than 25%, then it's better to stretch
			new_size = sizes[sizes.index(new_size) + 1]			# get the upper size, instead of a lower one
	return new_size

def resize_image(img, size):
	return img.resize((size, size), Image.ANTIALIAS)

def fill_image(img, fill_color=(255, 255, 255)):
	x, y = img.size
	max_size = max(x, y)
	new_img = Image.new('RGB', (max_size, max_size), fill_color)
	new_img.paste(img, (int((max_size - x) / 2), int((max_size - y) / 2)))
	return new_img

def stretch_image(img):
	x, y = img.size
	max_size = max(x, y)
	new_img = img.resize((max_size, max_size), Image.ANTIALIAS)
	return new_img

def make_square_jpg(folder_path):
	jpg_path = folder_path + "/jpg"
	jpg_fill_output = folder_path + "/jpg-fil"
	jpg_stretch_output = folder_path + "/jpg-str"
	for file in glob.glob(jpg_path + "/*"):
		file_name = os.path.basename(file)
		img = Image.open(file)
		x, y = img.size
		new_size = get_best_size(img)
		if x == y:
			img = resize_image(img, new_size) # I could check if the size was already ok and just copy
			img.save(jpg_fill_output + '/' + file_name)
			img.save(jpg_stretch_output + '/' + file_name)
		else:
			img_fil = fill_image(img)
			img_str = stretch_image(img)
			img_fil = resize_image(img_fil, new_size)
			img_str = resize_image(img_str, new_size)
			img_fil.save(jpg_fill_output + '/' + file_name)
			img_str.save(jpg_stretch_output + '/' + file_name)

def convert_png_to_jpg(folder_path):
	jpg_output_path = folder_path + "/jpg"
	for file in glob.glob(folder_path + "/*"):
		file_name = os.path.basename(os.path.splitext(file)[0])
		if file.endswith(tuple(png_ext)):
			img = Image.open(file)
			rgb_img = img.convert('RGB')
			rgb_img.save(jpg_output_path + '/' + file_name + ".jpg")
		elif file.endswith(tuple(jpg_ext)):
			shutil.copy(file, jpg_output_path + '/' + file_name + ".jpg")

def make_jpg_dirs(folder_path):
	jpg_output_path = folder_path + "/jpg"
	jpg_fill_output = folder_path + "/jpg-fil"
	jpg_stretch_output = folder_path + "/jpg-str"
	if not os.path.exists(jpg_output_path):
		os.mkdir(jpg_output_path)
	if not os.path.exists(jpg_fill_output):
		os.mkdir(jpg_fill_output)
	if not os.path.exists(jpg_stretch_output):
		os.mkdir(jpg_stretch_output)

def main():
	root_tk = tk.Tk()
	root_folder = get_root_folder(root_tk)
	make_jpg_dirs(root_folder)
	convert_png_to_jpg(root_folder)
	make_square_jpg(root_folder)
	root_tk.destroy()
    
if __name__ == "__main__":
    main()