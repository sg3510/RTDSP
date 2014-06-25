clc;
f = [260,450,2000,2200];
fs = 8000;
a = [0,1,0];
rp = 0.4;
sa=-50;
dev1 = (10^(rp/20)-1)/(10^(rp/20)+1);
dev2 = 10^(sa/20);
dev = [dev2 dev1 dev2];
[N,Fo,Ao,W]=firpmord(f,a,dev,fs);
B=firpm(N+8,Fo,Ao,W);
N+8
freqz(B,2048)
[h,w]=freqz(B,8192,7673);
%h=abs(h);
w=w.*4000/pi();
% plot(w.*800/2*pi(),10*log(abs(h)));grid on
% title('Frequency Response');
% xlabel('Frequency(Hz)');
% ylabel('Amplitude(dB)')
%plot(abs(h(60:250)))
%plot(10*log(abs(h(60:250))))
% stem(B)
% title('Values of coefficients'); grid on
% xlabel('Index');
% ylabel('Amplitude')
plot(w,log(h))
zplane(B)
startv=int16(75/512*7673);
endv=int16(220/512*7673);
disp(['The ripple is:' ,num2str(max(10*log(abs(h(startv:endv))))-min(10*log(abs(h(startv:endv)))))]);
save coef.txt B -ASCII -DOUBLE -TABS