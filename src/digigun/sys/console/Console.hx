package digigun.sys.console;

import cpp.Pointer;
import cpp.NativeArray;

/**
 * Class for low-level terminal and console control.
 */
class Console {
    /**
     * Toggles "Raw Mode" for the terminal.
     * @param enabled If true, enables raw mode. If false, restores previous mode.
     * @return True if successful.
     */
    public static function setRawMode(enabled:Bool):Bool {
        #if cpp
        return Native.set_raw_mode(enabled ? 1 : 0) == 1;
        #else
        return false;
        #end
    }

    /**
     * Retrieves the current terminal window dimensions.
     * @return An object containing width and height, or null if not available.
     */
    public static function getTerminalSize():{width:Int, height:Int} {
        #if cpp
        var w:Int = 0;
        var h:Int = 0;
        Native.get_size(Pointer.addressOf(w), Pointer.addressOf(h));
        if (w == -1 || h == -1) return null;
        return { width: w, height: h };
        #else
        return null;
        #end
    }

    /**
     * Writes a block of characters and color attributes to the console.
     * @param x Screen X coordinate.
     * @param y Screen Y coordinate.
     * @param width Width of the block.
     * @param height Height of the block.
     * @param chars Array of ASCII character codes.
     * @param attrs Array of platform-specific color attributes.
     */
    public static function writeBlock(x:Int, y:Int, width:Int, height:Int, chars:Array<Int>, attrs:Array<Int>):Void {
        #if cpp
        var nativeChars = NativeArray.address(chars, 0);
        var nativeAttrs = NativeArray.address(attrs, 0);
        Native.write_block(x, y, width, height, nativeChars.raw, nativeAttrs.raw);
        #end
    }

    /**
     * Reads a block of characters and color attributes from the console (Windows only).
     * @param x Screen X coordinate.
     * @param y Screen Y coordinate.
     * @param width Width of the block.
     * @param height Height of the block.
     * @return Object containing the read chars and attrs arrays.
     */
    public static function readBlock(x:Int, y:Int, width:Int, height:Int):{chars:Array<Int>, attrs:Array<Int>} {
        #if cpp
        var chars = new Array<Int>();
        var attrs = new Array<Int>();
        // Pre-initialize arrays
        for (i in 0...(width * height)) {
            chars.push(0);
            attrs.push(0);
        }
        var nativeChars = NativeArray.address(chars, 0);
        var nativeAttrs = NativeArray.address(attrs, 0);
        Native.read_block(x, y, width, height, nativeChars.raw, nativeAttrs.raw);
        return { chars: chars, attrs: attrs };
        #else
        return null;
        #end
    }

    /**
     * Toggles terminal cursor visibility.
     */
    public static function setCursorVisible(visible:Bool):Void {
        #if cpp
        Native.set_cursor_visible(visible ? 1 : 0);
        #end
    }

    /**
     * Switches to or from the alternate screen buffer.
     */
    public static function useAlternateBuffer(enable:Bool):Void {
        #if cpp
        Native.use_alternate_buffer(enable ? 1 : 0);
        #end
    }
}
