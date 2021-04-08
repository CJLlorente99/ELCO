#!/usr/local/bin/python3
from PIL import Image
import argparse
import os
import pathlib

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='generateRGBMatrix.py', description='Generate CSV files with RGB matrix info', formatter_class=lambda *args, **kwargs: argparse.HelpFormatter(*args, max_help_position=80, **kwargs))
    parser.add_argument('folderIn', type=str, help='Path to folder', metavar='Input')
    parser.add_argument('folderOut', type=str, help='Path to output folder (it is not necessary that it previously exists)', metavar='Output')
    parser.add_argument('-brightness', type=int, help='Pixel brightness', default=255, required=False, metavar='brightness')

    args = parser.parse_args()

    # Create folder for output files
    pathOut = str(pathlib.Path(__file__).parent.absolute()) + '/' + args.folderOut
    os.makedirs(pathOut, exist_ok=True)

    # Generate matrix for every file in folderIn
    pathIn = str(pathlib.Path(__file__).parent.absolute()) + '/' + args.folderIn
    files = os.listdir(args.folderIn)

    # Other choice is to write everything in just one file

    print('Writing allRGB.cpp file')
    with open(pathOut + '/allRGB.h', 'wt') as f:
        for file in files:
            img = Image.open(pathIn + str(file))
        
            f.write('int ' + file[:-4] + 'R[64] = {')
            for widthIndex in range(8):
                for heightIndex in range(8):
                    colors = img.getpixel((widthIndex, heightIndex))
                    if(widthIndex == 7 and heightIndex == 7):
                        if(colors[3] == 0):
                            f.write(str(0))
                        else:
                            f.write(str(abs(colors[0]-255)))
                    else:
                        if(colors[3] == 0):
                            f.write(str(0) + ', ')
                        else:
                            f.write(str(abs(colors[0]-255)) + ', ')
            f.write('};\n')

            f.write('int ' + file[:-4] + 'G[64] = {')
            for widthIndex in range(8):
                for heightIndex in range(8):
                    colors = img.getpixel((widthIndex, heightIndex))
                    if(widthIndex == 7 and heightIndex == 7):
                        if(colors[3] == 0):
                            f.write(str(0))
                        else:
                            f.write(str(abs(colors[1]-255)))
                    else:
                        if(colors[3] == 0):
                            f.write(str(0) + ', ')
                        else:
                            f.write(str(abs(colors[1]-255)) + ', ')
            f.write('};\n')

            f.write('int ' + file[:-4] + 'B[64] = {')
            for widthIndex in range(8):
                for heightIndex in range(8):
                    colors = img.getpixel((widthIndex, heightIndex))
                    if(widthIndex == 7 and heightIndex == 7):
                        if(colors[3] == 0):
                            f.write(str(0))
                        else:
                            f.write(str(abs(colors[2]-255)))
                    else:
                        if(colors[3] == 0):
                            f.write(str(0) + ', ')
                        else:
                            f.write(str(abs(colors[2]-255)) + ', ')
            f.write('};\n') 

            f.write('int ' + file[:-4] + 'Brightness[64] = {')
            for widthIndex in range(8):
                for heightIndex in range(8):
                    colors = img.getpixel((widthIndex, heightIndex))
                    if(widthIndex == 7 and heightIndex == 7):
                        f.write(str(args.brightness))  
                    else:
                        f.write(str(args.brightness) + ', ')
            f.write('};\n')

        print('Finished with ' + file)

    print('allRGB.cpp file written')