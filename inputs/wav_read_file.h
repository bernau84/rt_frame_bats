#ifndef WAV_IO_FILE_H
#define WAV_IO_FILE_H

#include <fstream>

//---------------------------------------------------------------------------
class t_waw_file_reader {

public:
    //program nekontroluje ale podporovany je jen format mono
    struct t_wav_header {

          char  		ascii_riff[4];
          unsigned long 	total_bytes;
          char  		ascii_wave[4];
          char              ascii_fmt[4];
          unsigned long 	pcm_10format;
          unsigned short 	pcm_01format;
          unsigned short 	num_channels;
          unsigned long 	sample_frequency;
          unsigned long 	bytes_per_second;
          unsigned short 	bytes_per_sample;
          unsigned short 	bites_per_sample;
          char  		ascii_data[4];
          unsigned long 	bytes_to_follow;
    };

private:
     std::ifstream in;
     bool cyclic;
     t_wav_header header __attribute__((packed));

public:
     /*! \brief - < 0 for invalid file, 0 for noncyclic reading at the end
      * bytes to end of file in other case
      */
     int avaiable(){

         if(!in.is_open())
             return -1;

         in.gcount();
     }

     /*! \brief - head copy in we have valid file; returns 0 for invalid
      * number of channels in other case
      */
     int info(t_waw_file_reader::t_wav_header &head){

        head = header;
        return header.num_channels;
     }

     /*! \brief - sequential read with data format to <-1, 1> double range
      */
     int read(double *dest, int size){

         if(!in.is_open())
             return -1;

         int overal = 0;
         while(overal < size){

             int psize = (size - overal) * header.bytes_per_sample;
             char data[psize]; //memset(data, '-', psize);

             in.read(data, psize);
             psize = in.gcount(); //number ob bytes from last get op

             unsigned char *data_08 = (unsigned char *)data;
             short         *data_16 = (short         *)data;

             switch( header.bytes_per_sample ){ //formatovani <-0.5, 0.5>

               case(1):
                 for(int i=0; i<psize; i++) dest[overal++] = data_08[i]/256.0/2 - 1.0;
               break;
               case(2):
                 for(int i=0; i<psize/2; i++) dest[overal++] = (data_16[i]/65536.0) * 2;
               break;
               default:
                 size = 0;
               break;
             }

             if(in.rdstate() & std::ifstream::eofbit){

                 if(cyclic){

                     in.clear();  // !clears fail and eof bits
                     in.seekg(0, in.beg);
                     in.read((char *)&header, sizeof(header));
                 } else {

                     break;
                 }
             }
         }

         return overal;
     }


     t_waw_file_reader(const char *path, bool auto_rewind):
        in(path, std::ifstream::binary),
        cyclic(auto_rewind)
     {
        memset(&header, 0, sizeof(header));
        if(in.is_open()){

            in.read((char *)&header, sizeof(header));
            if(in.rdstate() & std::ifstream::failbit)
                in.close();
        }
     }

     ~t_waw_file_reader(){

         if(in.is_open())
            in.close();
     }
};


#endif // WAV_IO_FILE_H

