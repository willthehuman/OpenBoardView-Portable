using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Http;
using System.Numerics;
using System.Text;
using Xamarin.Forms.Shapes;

namespace OBV
{
    internal class BRDFile
    {
        static readonly Byte[] BRD_FILE_HEADER = { 0x23, 0xe2, 0x63, 0x28 };
        public BRDFile(Stream buffer)
        {
            buffer.Position = 0;

            // Read the first 4 bytes of the file
            Byte[] fileHeader = new Byte[buffer.Length];
            buffer.Read(fileHeader, 0, (int)buffer.Length);

            // Check if the file is a valid .brd file by checking the header
            for (int i = 0; i < 4; i++)
            {
                if (fileHeader[i] != BRD_FILE_HEADER[i])
                {
                    throw new Exception("Invalid .brd file");
                }
            }

            var memStream = new MemoryStream();

            // decode the file
            foreach (byte b in fileHeader)
            {
                // convert byte to char
                char x = (char)b;
                if (x == '\r' || x == '\n' || x == 0)
                {
                    // ignore
                }
                else
                {
                    // decode
                    int c = x;
                    x = (char)~(((c >> 6) & 3) | (c << 2));
                    // write x to stream
                    Console.Write(x);
                    memStream.WriteByte((byte)x);
                }
            }

            memStream.Position = 0;
            var reader = new StreamReader(memStream);

            // Read the memStream line by line
            String line;            
            int currentBlock = 0;
            while ((line = reader.ReadLine()) != null)
            {
                // split on multiple spaces
                String[] lineSplit = line.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

                // remove what's after the colon
                var test = lineSplit[0].Split(':')[0];

                if (test == "str_length")
                {
                    currentBlock = 1;
                    continue;
                }

                if (test == "var_data")
                {
                    currentBlock = 2;
                    continue;
                }

                if (test == "Format" || test == "format")
                {
                    currentBlock = 3;
                    continue;
                }

                if (test == "Parts" || test == "Pins1")
                {
                    currentBlock = 4;
                    continue;
                }

                if (test == "Pins" || test == "Pins2")
                {
                    currentBlock = 5;
                    continue;
                }

                if (test == "Nails")
                {
                    currentBlock = 6;
                    continue;
                }
            }
        }
    }
}
