using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;
using System.Text;

namespace OBV
{
    internal class BRDFile
    {
        static readonly Byte[] BRD_FILE_HEADER = { 0x23, 0xe2, 0x63, 0x28 };
        public BRDFile(Stream buffer)
        {
            var fileBuffer = new StreamReader(buffer, Encoding.UTF8);
            fileBuffer.BaseStream.Position = 0;

            // Read the first 4 bytes of the file
            Byte[] fileHeader = new Byte[4];
            fileBuffer.BaseStream.Read(fileHeader, 0, 4);

            // Check if the file is a valid .brd file by checking the header
            for (int i = 0; i < 4; i++)
            {
                if (fileHeader[i] != BRD_FILE_HEADER[i])
                {
                    throw new Exception("Invalid .brd file");
                }
            }

            
        }

        public string FileName { get; internal set; }
        public string FilePath { get; internal set; }
        public string FileContent { get; internal set; }
    }
}
