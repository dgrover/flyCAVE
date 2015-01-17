function image = stripes(stripeWidth);
image=ones(360);
for i=1:(2*stripeWidth):360;

image(1:360, i:i+stripeWidth-1)=0;
end
imshow(image);
end