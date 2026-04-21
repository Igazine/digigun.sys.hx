package digigun.sys;

/**
 * Internal class to handle native build configuration.
 * Inclusion of this class ensures that Build.xml is processed by HXCPP.
 */
#if cpp
@:buildXml('<include name="../../../native/Build.xml" />')
@:keep
class NativeBuild {
    /**
     * Dummy method to ensure the class is referenced and not stripped by the compiler.
     */
    public static function init():Void {}
}
#end
