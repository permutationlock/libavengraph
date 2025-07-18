# pull android jar file for given version
rm -f android-$ANDROID_VERSION.jar
wget https://github.com/Sable/android-platforms/raw/refs/heads/master/android-$ANDROID_VERSION/android.jar -O android-$ANDROID_VERSION.jar
