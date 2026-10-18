// Synthetic vowel/chord preview of the actual C++ vocoder + FX, not reference audio.
#include "../VocoderDsp.h"
#include "../GranularFx.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

static float PolyBlep(float t, float dt)
{
    if(t < dt) { t /= dt; return 2*t-t*t-1; }
    if(t > 1-dt) { t = (t-1)/dt; return t*t+2*t+1; }
    return 0;
}
static void U32(FILE* f, uint32_t n)
{
    for(int i=0;i<4;++i) std::fputc((n>>(8*i))&255,f);
}
static void U16(FILE* f, uint16_t n)
{
    std::fputc(n&255,f);std::fputc(n>>8,f);
}
int main(int argc, char** argv)
{
    if(argc != 2) return 1;
    FILE* file = std::fopen(argv[1],"wb");
    if(!file) return 2;
    constexpr int sr=48000, seconds=15, count=sr*seconds;
    std::fwrite("RIFF",1,4,file);U32(file,36+count*4);std::fwrite("WAVEfmt ",1,8,file);
    U32(file,16);U16(file,1);U16(file,2);U32(file,sr);U32(file,sr*4);U16(file,4);U16(file,16);
    std::fwrite("data",1,4,file);U32(file,count*4);
    std::vector<float> memory(GranularFx::kStorageSize);
    GranularFx fx; VocoderDsp vocoder;
    float phase[4]={};
    const float frequencies[4]={130.8128f,164.8138f,195.9977f,261.6256f};
    for(int i=0;i<count;++i)
    {
        const int segment=i/(5*sr), local=i%(5*sr);
        if(local==0) { fx.Init(sr,memory.data());vocoder=VocoderDsp{};vocoder.Init(sr); }
        if(i%48==0) fx.SetControls(segment>0,segment==1?.5f:1.f);
        float car=0;
        for(int v=0;v<4;++v)
        {
            float dt=frequencies[v]/sr;
            car+=.18f*(2*phase[v]-1-PolyBlep(phase[v],dt));
            phase[v]+=dt;if(phase[v]>=1)phase[v]-=1;
        }
        const float t=float(local)/sr;
        const float syllable=std::fmod(t,.65f);
        const float gate= t<3.3f ? std::min(1.f,syllable/.025f)*std::max(0.f,std::min(1.f,(.48f-syllable)/.08f)):0;
        float mic=0;
        const float f1=t<1.6f?700.f:400.f, f2=t<1.6f?1200.f:2200.f;
        for(int h=1;h<=48;++h)
        {
            float f=110.f*h;
            float amplitude=(std::exp(-.5f*std::pow((f-f1)/120,2))+
                             .7f*std::exp(-.5f*std::pow((f-f2)/180,2)))/h;
            mic+=amplitude*std::sin(6.28318530718f*f*t);
        }
        float source=vocoder.Process(.22f*mic*gate,car,0,.55f);
        auto out=fx.Process(source);
        U16(file,uint16_t(int16_t(std::round(out.left*32767))));
        U16(file,uint16_t(int16_t(std::round(out.right*32767))));
    }
    std::fclose(file);
}
