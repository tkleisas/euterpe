# JNI entry points are resolved by their exact Java/Kotlin class and method names.
-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}
-keep class gr.tkleisas.euterpe.audio.EuterpeEngine { *; }
