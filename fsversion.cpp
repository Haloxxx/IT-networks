#include <filesystem>
#include <iostream>
#include <vector>
#include <fstream>
#include <span>
#include <random>
#include <map>

namespace fs = std::filesystem;


template<class T>
char* as_bytes(T& i) 
{
    void* addr = &i; 
    return static_cast<char*>(addr);
}


std::vector<uint8_t> readFile(std::string file_path)
{
    fs::path p = fs::current_path()/file_path;
    int size = fs::file_size(p)+1;

    std::vector<uint8_t> memblock(size);

    std::ifstream file (file_path, std::ios_base::binary);

    if(file.is_open())
    {
        int for_loop_iterator = 0;
        for(uint8_t x; file.read(as_bytes(x),sizeof(uint8_t));)
            {memblock[for_loop_iterator]=x; ++for_loop_iterator;}

    } else std::cout<<"Nie udało się\n";

    return memblock;
}

int writeFile(std::string file_path, std::vector<uint8_t> memblock)
{
    std::ofstream file (file_path, std::ios_base::binary);

    for(uint8_t x : memblock)
        file.write(as_bytes(x), sizeof(uint8_t));

    return 0;
}

uint8_t parity_bit (const std::vector<uint8_t> &memblock)
{
    int result = 0;

    for(int j =0; j<memblock.size(); ++j)
    {
        uint8_t temporary = 1;
        for(int i = 0; i<8; ++i)
        {
            if((memblock[j] | temporary) != memblock[j]) ++result;
            temporary<<=1; 
        }
    }
    
    return result%2;
}

uint8_t modulo_sum (const std::vector<uint8_t> &memblock)
{
    int value = 2;
    int addon = -1;
    int result = 0;

    for(int j =0; j<memblock.size(); ++j)
    {
        result+=value*memblock[j];
            
        value +=addon;
        addon*=-1;

        if(j%100==0) result%=10;
    }

    return result%10;
}

std::vector<uint8_t> crc (const std::vector<uint8_t> &memblock, int poly)
{
    int result = 0;
    int temp = poly;
    int one = 1;
    uint8_t h_eight = 128;
    int first_bit = 0;
    int last_bit = 7;

    while(!(poly&one))
    {
        one = one<<1;
        ++first_bit;
    }

    while(!(poly&h_eight))
    {
        h_eight = h_eight>>1;
        last_bit--;
    }

    std::vector<uint8_t> resultblock = memblock;
    resultblock.push_back(0);


    for(int q = 0; q<memblock.size(); ++q)
    {

        while(resultblock[q]!=0)
        {
            temp = poly;
            int i =0, j= 0;
            int block_power = 7;
            h_eight = 128;

            uint8_t blockcopy = resultblock[q];

            while(!(blockcopy&h_eight))
            {
                h_eight = h_eight>>1;
                block_power--;
            }
            
            if(block_power>last_bit) i = block_power-last_bit;
            else j = last_bit - block_power;

            temp = temp << i;
            temp = temp >> j;
        
            resultblock[q] = resultblock[q] xor temp;

            if(first_bit-j<0) resultblock[q+1] = resultblock[q+1] xor poly<<(8-j);
        }
    }

    result = resultblock[memblock.size()]>>(last_bit+1);

    resultblock.back()=result;

    return resultblock;
}

std::vector<uint8_t> error1(const std::vector<uint8_t> &memblock, double percentage)
{

    std::vector<uint8_t> result = memblock;

    int error_size = 8 * memblock.size() * percentage;

    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, memblock.size()-1);
    std::uniform_int_distribution<int> bit(0, 7);

    for(int i =0; i<error_size; ++i)
    {
        int error = 1;
        result[dist(rd)]=memblock[dist(rd)] xor error<<bit(rd);
    }

    return result;
}

std::vector<uint8_t> error2(const std::vector<uint8_t> &memblock, double percentage)
{
    std::vector<uint8_t> result = memblock;

    int error_size = 8 * memblock.size() * percentage;

    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, memblock.size()-1);
    std::uniform_int_distribution<int> bit(0, 7);

    std::vector<std::vector<bool>> bits (memblock.size(), std::vector<bool>(8));

    for(int i =0; i<error_size; ++i)
    {
        int error = 1;


        int byte_no = dist(rd);
        int bit_no = bit(rd);

        if(bits[byte_no][bit_no]==false)
        {
            bits[byte_no][bit_no]=true;
            result[dist(rd)]=memblock[dist(rd)] xor error<<bit(rd);
            i--;
        }
    }

    return result;
}


int main()
{
    int input = 0;
    int poly = 12;
    uint8_t result = 0;
    std::vector<uint8_t> memblock = readFile("test.pdf");

    std::cout<<"Wybierz sumę kontrolną: \n";
    std::cin>>input;

    switch(input)
    {
        case 1:
        {
            result = parity_bit(memblock);
            break;
        }

        case 2:
        {
            result = modulo_sum(memblock);
            break;
        }

        case 3:
        {
            result = crc(memblock, poly).back();
            break;
        }
    }

    memblock.push_back(result);
    writeFile("wynik.bin",memblock);

    std::vector<uint8_t> no_error = readFile("wynik.bin");

    std::vector<uint8_t> repeating_error = error1(no_error, 0.1);

    /*
    uint8_t result2 = 0;
    result2 = crc(repeating_error, 12).back();
    std::cout<<"result2 = "<<int(result2)<<'\n';
    */////

    std::vector<uint8_t> nonrepeating_error = error2(no_error, 0.1);

    uint8_t c_sum = repeating_error.back(); 
    uint8_t c_sum2 = nonrepeating_error.back(); 

    repeating_error.pop_back();
    nonrepeating_error.pop_back(); 

    uint8_t result2 = 0;
    uint8_t result3 = 0;


    switch(input)
    {
        case 1:
        {
            result2 = parity_bit(repeating_error);
            result3 = parity_bit(nonrepeating_error);
            break;
        }

        case 2:
        {
            result2 = modulo_sum(repeating_error);
            result3 = modulo_sum(nonrepeating_error);
            break;
        }

        case 3:
        {
            result2 = crc(repeating_error, poly).back();
            result3 = crc(nonrepeating_error, poly).back();
            break;
        }
    }

    if(c_sum!=result2) std::cout<<"Błąd powtarzający się. Transmisja nieudana.\n";
    if(c_sum2!=result3) std::cout<<"Błąd niepowtarzający się. Transmisja nieudana.\n";

}