"""Real-vocal check for ThroatTexture. Usage: python3 tests/real_audio_throat.py a.wav [b.wav ...]
Builds tests/throat_wav.cpp, runs 0/33/67/100 %, asserts: bit-exact clean, |level change|<1 dB, finite, peak growth<3 dB."""
import numpy as np,struct,subprocess,sys,os
here=os.path.dirname(os.path.abspath(__file__))
subprocess.run(["g++","-O2","-std=c++17",os.path.join(here,"throat_wav.cpp"),"-o","/tmp/throat_wav"],check=True)
def load(p):
    b=open(p,'rb').read();i=12;fmt=None
    while i<len(b):
        cid=b[i:i+4];sz=struct.unpack('<I',b[i+4:i+8])[0]
        if cid==b'fmt ':fmt=struct.unpack('<HHIIHH',b[i+8:i+24])
        if cid==b'data':
            dt={(3,32):'<f4',(1,16):'<i2'}[(fmt[0],fmt[5])]
            a=np.frombuffer(b[i+8:i+8+sz],dtype=dt).astype(np.float32)
            return fmt[2],(a/32768 if dt=='<i2' else a).reshape(-1,fmt[1])
        i+=8+sz+(sz&1)
for p in sys.argv[1:]:
    sr,x=load(p);st=np.repeat(x,2,1) if x.shape[1]==1 else x[:,:2];L=st[:,0]
    st.astype('<f4').tofile("/tmp/in.raw")
    for a in (0,.3333,.6667,1.):
        subprocess.run(["/tmp/throat_wav","/tmp/in.raw","/tmp/o.raw",str(sr),str(a)],check=True)
        y=np.fromfile("/tmp/o.raw",dtype='<f4').reshape(-1,2)[:,0]
        assert np.isfinite(y).all()
        if a==0: assert np.array_equal(y,L),"clean must be bit-exact"
        act=np.abs(L)>.02;lv=20*np.log10(np.sqrt(np.mean(y[act]**2))/np.sqrt(np.mean(L[act]**2)))
        pk=20*np.log10(np.abs(y).max()/np.abs(L).max())
        assert abs(lv)<1 and pk<3,(p,a,lv,pk)
        print(f"{os.path.basename(p)} {a*100:3.0f}%: level {lv:+.2f} dB peak {pk:+.2f} dB")
print("PASS")
