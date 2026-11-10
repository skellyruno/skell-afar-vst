#include "../Source/dsp/ThroatTexture.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
int main(int c,char**v){ // throat_wav in.raw out.raw sr amount  (interleaved stereo float)
  FILE*f=fopen(v[1],"rb");std::vector<float> d;float s;while(fread(&s,4,1,f)==1)d.push_back(s);fclose(f);
  afar::ThroatTexture t;t.prepare(atof(v[3]));t.setAmount(float(atof(v[4])));
  for(size_t i=0;i+1<d.size();i+=2){auto o=t.process(d[i],d[i+1]);d[i]=o.left;d[i+1]=o.right;}
  f=fopen(v[2],"wb");fwrite(d.data(),4,d.size(),f);fclose(f);}
