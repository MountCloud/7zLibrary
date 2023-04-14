
#include "util/lzmautil.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>


void testMemByte(){

    //test data
    std::string data = "start:AnemptystreetAnemptyhouseAholeinsidemyheartI'mallaloneandtheroomsaregettingsmallerIwonderhow,IwonderwhyIwonderwheretheyareThedayswehad,thesongswesangtogether(ohyeah)AndohmyloveI'mholdingonforeverReachingforalovethatseemssofarSoIsayalittleprayerAndhopemydreamswilltakemethereWheretheskiesareblueToseeyouonceagain,myloveOverseasfromcoasttocoastTofindtheplaceIlovethemostWherethefieldsaregreenToseeyouonceagain,myloveItrytoreadIgotoworkI'mlaughingwithmyfriendsButIcan'tstoptokeepmyselffromthinking(ohno)Iwonderhow,IwonderwhyIwonderwheretheyareThedayswehad,thesongswesangtogether(ohyeah)AndohmyloveI'mholdingonforeverReachingforalovethatseemssofarSoIsayalittleprayer,Andhopemydreamswilltakemethere,Wheretheskiesarebluetoseeyouonceagain,Mylove,Overseasfromcoasttocoast,TofindtheplaceIlovethemost,Wherethefieldsaregreentoseeyouonceagain,ToholdyouinmyarmsTopromiseyoumyloveTotellyoufromtheheartYou'reallI'mthinkingofReachingforalovethatseemssofarSoIsayalittleprayerAndhopemydreamswilltakemethereWheretheskiesareblueToseeyouonceagain,myloveOverseasfromcoasttocoastTofindtheplaceIlovethemostWherethefieldsaregreenToseeyouonceagain,myloveSeeyoulittleprayermustbedreamswilltakemethereWheretheskiesareblueToseeyouonceagainOverseasfromcoasttocoastTofindtheplaceIlovethemostWherethefieldsaregreenToseeyouonceagain,mylove";

    
    mc::ByteStream encodeBs;
    LzmaUtil::lzmaCompress((const uint8_t*)data.c_str(), data.length(), &encodeBs);

    std::cout << "compress size:" << encodeBs.size() << std::endl;

    mc::ByteStream decodeBs;
    mc::BS_DSIZE encodeSize = encodeBs.size();
    char* encodeData = (char*)calloc(encodeSize, sizeof(char));
    encodeBs.read(encodeData, encodeSize);

    LzmaUtil::lzmaDecompress((const uint8_t*)encodeData, encodeSize, &decodeBs);

    std::cout << "decompress size:" << decodeBs.size() << std::endl;

    char* decodeData = (char*)calloc(decodeBs.size()+1, sizeof(char));
    decodeBs.read(decodeData, decodeBs.size());

    std::cout << "decompress data:" << decodeData << std::endl;

}

#ifdef _WIN32
void testFile(){
    std::string infilepath = "D:\\test.txt";
    std::string outfilepath = "D:\\test.txt.out";

    LZMA_DATASIZE inputsize;
    LZMA_DATASIZE outsize = LzmaUtil::EncodeFile(infilepath.c_str(),outfilepath.c_str(),&inputsize);
    std::cout << "inputsize:" << inputsize << ",outsize:" << outsize << std::endl;
}
#else

#endif

int main(){

    testMemByte();

    testFile();

    return 0;
}