clc; %clear screen
% get coefficient values
% passband values must be normalized to Nyquist frequency
% and the function should be used such as:
% ellip(order, ripple (dB), stopband attenuation, passband frequencies)
[b,a] = ellip(3,.5,25,[280/4000 460/4000]); 
figure(1); %intialize first window
[h,w]=freqz(b,a,1204,8000); %plot graph of gaina and phase
freqz(b,a,1204,8000);
h=angle(h)*180/pi;
figure(2); %intialize second window
% view plot of zeros and poles to understand potential
% effect of precision of coefficients on filter performance
zplane(b,a); 
formatSpec = '%1.16x,'; %set format to high precision to avoid rounding issues
fid=fopen('ccs_proj\RTDSP\coef.txt','w'); %open file
% define order of filter+1 here
% simpler and more intuitive way than
% playing around with N in C
fprintf(fid,'#define N %d\n', length(a)); 
%define a and b coefficient arrays
fprintf(fid,'double a[] = {');
fprintf(fid,formatSpec,a(1:length(a)-1));
fprintf(fid,'%1.16x};\n',a(length(a)));
fprintf(fid,'double b[] = {');
fprintf(fid,formatSpec,b(1:length(b)-1));
fprintf(fid,'%1.16x};\n',b(length(b)));
fclose(fid); %close file
