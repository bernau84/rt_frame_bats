#ifndef WAV_WRITE_FILE
#define WAV_WRITE_FILE


#include <fstream>

//---------------------------------------------------------------------------
class t_waw_file_writer {

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
     std::ofstream out;
     t_wav_header header __attribute__((packed));

     void update_sz(unsigned total){

         header.total_bytes = total;
         header.bytes_to_follow = total - sizeof(t_wav_header);

         out.seekp(0, out.beg);
         out.write((char *)&header, sizeof(t_wav_header));
         out.seekp(0, out.end);
     }

public:
     /*! \brief - head copy in we have valid file; returns 0 for invalid
      * number of channels in other case
      */
     int info(t_waw_file_writer::t_wav_header &head){

        head = header;
        return header.num_channels;
     }

     /*! \brief - sequential write data in double format <-1, 1> to integer wav
      * size is equal to total number of samples
      */
     int write(double *data, int size){

         if(!out.is_open())
             return -1;

         for(int i=0; i<size; i++){

            switch( header.bytes_per_sample ){ //formatovani <-0.5, 0.5>
                case(1): {
                    unsigned char data_08 = (data[i] + 1.0)*256/2;
                    out.write((char *)&data_08, 1);
                }
                break;
                case(2): {
                    unsigned char data_16 = (data[i] + 1.0)*65536/2;
                    out.write((char *)&data_16, 2);
                }
                break;
                default:
                    return 0;
                break;
            }
         }

         update_sz(header.total_bytes + size);
         return size;
     }


     t_waw_file_writer(const char *path, unsigned int fs, unsigned short channels = 1, unsigned short precision = 16):
        out(path, std::ifstream::binary)
     {
        t_wav_header iheader = {

           {'R','I','F','F'},       //        char  		ascii_riff[4];
           sizeof(t_wav_header),    //        unsigned long 	total_bytes;
           {'W','A','V','E'},       //        char  		ascii_wave[4];
           {'f','m','t',' '},       //        char              ascii_fmt[4];
           16,                      //        unsigned long 	pcm_10format;
           1,                       //        unsigned short 	pcm_01format;
           channels,                //        unsigned short 	num_channels;
           fs,                      //        unsigned long 	sample_frequency;
           (fs * precision * channels) / 8,             //        unsigned long 	bytes_per_second;
           (unsigned short)((precision * channels) / 8),//        unsigned short 	bytes_per_sample;
           precision,               //        unsigned short 	bites_per_sample;
           {'d','a','t','a'},       //        char  		ascii_data[4];
           0                        //unsigned long 	bytes_to_follow;
        };

        header = iheader;
        update_sz(header.total_bytes);
     }

     ~t_waw_file_writer(){

         if(out.is_open())
            out.close();
     }
};

#endif // WAV_WRITE_FILE

