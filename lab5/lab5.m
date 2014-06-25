clc;
R=1000;
C=10^-6;
Ts=1/8000;
a=(2/Ts)*R*C;
B=[1 1];
A=[1+a 1-a];
%b=[1 1];
%a=[a_v+1 1-a_v];
[h,w]=freqz(B,A,1216,8000);
freqz(B,A,1216,8000);
%plot(w,angle(h).*180/pi())
angle_h=angle(h).*180/pi();
gain=20.*log10(abs(h));
%plot(gain)
%fvtool(B,A);
%freqz(B,A,2^10,8000);
plot(w,20*log10(abs(h))); grid on
B = B./A(1);
A = A./A(1);