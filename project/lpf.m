clc;
a = ones(256,1);
%choose what factor to set the values to
%and where this should start
lp_ampli = 1.5;
lp_freq = 10;
hp_ampli = 1.2;
hp_freq = 30;
for i=0:(lp_freq-1)
    a(i+1) = (lp_freq*lp_ampli-(lp_ampli-1)*i)/lp_freq;
    a(256-i) = (lp_freq*lp_ampli-(lp_ampli-1)*i)/lp_freq;
end
a_eq = (hp_ampli-1)/(128-hp_freq);
b_eq = 1 - a_eq*hp_freq;
for i=hp_freq:128
    a(i+1) = a_eq*i+b_eq;
    a(257-i) = a_eq*i+b_eq;
end
plot(1:256,a)
axis tight
title('Alpha exaggeration coefficients')
xlabel('Frequency index')
ylabel('Amplification factor')
formatSpec = '%1.16f,'; %set format to high precision to avoid rounding issues
fid=fopen('project_pt1\RTDSP\lpfcoef.txt','w'); %open file
% define order of filter+1 here
% simpler and more intuitive way than

%define a and b coefficient arrays
fprintf(fid,'float lpfcoef[] = {');
fprintf(fid,formatSpec,a(1:length(a)-1));
fprintf(fid,'%1.16f};\n',a(length(a)));
fclose(fid); %close file