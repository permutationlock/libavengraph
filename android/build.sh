# required environment:
#     ANDROID_VERSION: the target android version number (must be >=22)
#     ANDROID_JAR: a path to the android.jar file (see pull_android_jar.sh)
#     ANDROID_AAPT: path to android sdk aapt
# #     ANDROID_AAPT2: path to android sdk aapt2
#     ANDROID_ZIPALIGN: path to android sdk zipalign
#     ANDROID_APKSIGNER: path to android sdk apksizinger
#     ANDROID_CLANG: path to android ndk clang
#     APP_NAME: the name of your app
#     ORG_NAME: the name of your organization
#     KEYSTORE_FILE: path to keystore file
#     STORE_PASS: keystore password
#     KEY_PASS: key password
#     CFLAGS: common flags to be passed to C compiler
#     LDFLAGS: common flags to be passed to the linker

# clean old build artifacts
./clean.sh

# initualize build directory from template
cp -r template build_android
envsubst '$$ANDROID_VERSION $$APP_NAME $$ORG_NAME' < template/AndroidManifest.xml > build_android/AndroidManifest.xml

# build so for arm64
cd ..
./build clean --android
./build --android --cc $ANDROID_CLANG --ldflags "--target=aarch64-linux-android22 $LDFLAGS" --ccflags "--target=aarch64-linux-android22 $CFLAGS" --glfw-ccflags "--target=aarch64-linux-android22 $CFLAGS" --stb-ccflags "--target=aarch64-linux-android22 $CFLAGS" --android-ccflags "--target=aarch64-linux-android22 $CFLAGS"
cd android

mkdir -p build_android/apk/lib/arm64-v8a
cp ../build_out/visualization.so build_android/apk/lib/arm64-v8a/libmain.so

# build so for arm32
cd ..
./build clean --android
./build --android --cc $ANDROID_CLANG --ldflags "--target=armv7a-linux-androideabi22 $LDFLAGS" --ccflags "--target=armv7a-linux-androideabi22 $CFLAGS" --glfw-ccflags "--target=armv7a-linux-androideabi22 $CFLAGS" --stb-ccflags "--target=armv7a-linux-androideabi22 $CFLAGS" --android-ccflags "--target=armv7a-linux-androideabi22 $CFLAGS"
cd android

mkdir -p build_android/apk/lib/armeabi-v7a
cp ../build_out/visualization.so build_android/apk/lib/armeabi-v7a/libmain.so

# build so for x86
cd ..
./build clean --android
./build --android --cc $ANDROID_CLANG --ldflags "--target=i686-linux-android22 $LDFLAGS" --ccflags "--target=i686-linux-android22 $CFLAGS" --glfw-ccflags "--target=i686-linux-android22 $CFLAGS" --stb-ccflags "--target=i686-linux-android22 $CFLAGS" --android-ccflags "--target=i686-linux-android22 $CFLAGS"
cd android

mkdir -p build_android/apk/lib/x86
cp ../build_out/visualization.so build_android/apk/lib/x86/libmain.so

# build for x86_64
cd ..
./build clean --android
./build --android --cc $ANDROID_CLANG --ldflags "--target=x86_64-linux-android22 $LDFLAGS" --ccflags "--target=x86_64-linux-android22 $CFLAGS" --glfw-ccflags "--target=x86_64-linux-android22 $CFLAGS" --stb-ccflags "--target=x86_64-linux-android22 $CFLAGS" --android-ccflags "--target=x86_64-linux-android22 $CFLAGS"
cd android

mkdir -p build_android/apk/lib/x86_64
cp ../build_out/visualization.so build_android/apk/lib/x86_64/libmain.so

# build temporary apk and unzip back to directory
$ANDROID_AAPT package -f -F build_android/temp.apk -I $ANDROID_JAR -M build_android/AndroidManifest.xml -S build_android/apk/res -v --target-sdk-version $ANDROID_VERSION
unzip -o build_android/temp.apk -d build_android/apk

# zip compress second temporary apk
cd build_android/apk
zip -D4r ../temp2.apk .
zip -D0r ../temp2.apk ./resources.arsc ./AndroidManifest.xml
cd ../..

# align and sign final apk
$ANDROID_ZIPALIGN -v 4 build_android/temp2.apk build_android/$APP_NAME.apk
$ANDROID_APKSIGNER sign --key-pass pass:$KEY_PASS --ks-pass pass:$STORE_PASS --ks $KEYSTORE_FILE build_android/$APP_NAME.apk
