package digigun.sys.dl;

import haxe.macro.Context;
import haxe.macro.Expr;
import haxe.macro.Type;

/**
 * Advanced FFI automation for Dynamic Symbol Loading.
 */
class FFI {
    /**
     * Build macro for classes marked with @:struct to automate C-struct layout mapping.
     */
    public static macro function struct():Array<Field> {
        var fields = Context.getBuildFields();
        var newFields:Array<Field> = [];
        var currentOffset = 0;
        var maxAlign = 1;

        // Add a marker meta to the class itself so the build() macro can find it
        var localClass = Context.getLocalClass();
        if (localClass != null) {
            localClass.get().meta.add(":ffi_struct", [], Context.currentPos());
        }

        for (field in fields) {
            switch (field.kind) {
                case FVar(t, _):
                    var typeName = haxe.macro.ComplexTypeTools.toString(t);
                    var size = 0;
                    var align = 1;

                    switch (typeName) {
                        case "Int": size = 4; align = 4;
                        case "Float": size = 8; align = 8;
                        case "Bool": size = 1; align = 1; 
                        case "String": size = 8; align = 8;
                        default: size = 8; align = 8;
                    }

                    if (align > maxAlign) maxAlign = align;

                    if (currentOffset % align != 0) {
                        currentOffset += (align - (currentOffset % align));
                    }

                    var fieldOffset = currentOffset;
                    currentOffset += size;

                    var getterName = "get_" + field.name;
                    var setterName = "set_" + field.name;

                    var getterExpr = switch (typeName) {
                        case "Int": macro return _bytes.getInt32($v{fieldOffset});
                        case "Float": macro return _bytes.getDouble($v{fieldOffset});
                        case "Bool": macro return _bytes.get($v{fieldOffset}) != 0;
                        case "String": 
                            macro {
                                var ptr:cpp.ConstCharStar = untyped __cpp__("*(const char**)((char*){0} + {1})", _getPointer(), $v{fieldOffset});
                                return (ptr == null) ? null : ptr.toString();
                            };
                        default: macro return null;
                    }

                    var setterExpr = switch (typeName) {
                        case "Int": macro { _bytes.setInt32($v{fieldOffset}, value); return value; };
                        case "Float": macro { _bytes.setDouble($v{fieldOffset}, value); return value; };
                        case "Bool": macro { _bytes.set($v{fieldOffset}, value ? 1 : 0); return value; };
                        case "String":
                            macro {
                                untyped __cpp__("*(const char**)((char*){0} + {1}) = {2}", _getPointer(), $v{fieldOffset}, (cast value : cpp.ConstCharStar));
                                return value;
                            };
                        default: macro return value;
                    }

                    newFields.push({
                        name: field.name,
                        access: [APublic],
                        kind: FProp("get", "set", t),
                        pos: field.pos
                    });

                    newFields.push({
                        name: getterName,
                        access: [APrivate],
                        kind: FFun({
                            args: [],
                            ret: t,
                            expr: getterExpr
                        }),
                        pos: field.pos
                    });

                    newFields.push({
                        name: setterName,
                        access: [APrivate],
                        kind: FFun({
                            args: [{name: "value", type: t}],
                            ret: t,
                            expr: setterExpr
                        }),
                        pos: field.pos
                    });

                default:
                    newFields.push(field);
            }
        }

        if (currentOffset % maxAlign != 0) {
            currentOffset += (maxAlign - (currentOffset % maxAlign));
        }
        var structSize = currentOffset;

        newFields.push({
            name: "_bytes",
            access: [APrivate],
            kind: FVar(macro : haxe.io.Bytes),
            pos: Context.currentPos()
        });

        newFields.push({
            name: "new",
            access: [APublic],
            kind: FFun({
                args: [{name: "existing", type: macro : haxe.io.Bytes, opt: true}],
                ret: null,
                expr: macro {
                    if (existing != null) _bytes = existing;
                    else _bytes = haxe.io.Bytes.alloc($v{structSize});
                }
            }),
            pos: Context.currentPos()
        });

        newFields.push({
            name: "address",
            access: [APublic],
            kind: FProp("get", "never", macro : haxe.Int64),
            pos: Context.currentPos()
        });

        newFields.push({
            name: "get_address",
            access: [APrivate],
            kind: FFun({
                args: [],
                ret: macro : haxe.Int64,
                expr: macro return untyped __cpp__("(long long)(size_t){0}", _getPointer())
            }),
            pos: Context.currentPos()
        });

        newFields.push({
            name: "_getPointer",
            meta: [{name: ":noCompletion", pos: Context.currentPos()}],
            access: [APublic],
            kind: FFun({
                args: [],
                ret: macro : cpp.Star<cpp.Void>,
                expr: macro return cast untyped __cpp__("(void*)&{0}->b[0]", _bytes)
            }),
            pos: Context.currentPos()
        });

        return newFields;
    }

    /**
     * Build macro to automate dynamic library bindings with smart type conversion.
     */
    public static macro function build():Array<Field> {
        var fields = Context.getBuildFields();
        var newFields:Array<Field> = [];

        var localClass = Context.getLocalClass().get();
        var className = localClass.name;
        var libVarName = "_lib_handle_" + className;

        for (field in fields) {
            var nativeMeta = null;
            for (m in field.meta) if (m.name == ":native") nativeMeta = m;

            if (nativeMeta != null && nativeMeta.params.length > 0) {
                var nativeNameExpr = nativeMeta.params[0];
                var nativeName = switch (nativeNameExpr.expr) {
                    case EConst(CString(s)): s;
                    default: "";
                }
                
                switch (field.kind) {
                    case FFun(f):
                        var fieldName = field.name;
                        var wrapperName = "_" + fieldName + "_ptr";
                        
                        field.meta = [for (m in field.meta) if (m.name != ":native") m];

                        newFields.push({
                            name: wrapperName,
                            access: [APrivate, AStatic],
                            kind: FVar(macro : haxe.Int64, macro (0 : haxe.Int64)),
                            pos: field.pos
                        });

                        var callArgs = [];
                        var cppArgTypes = [];
                        for (arg in f.args) {
                            var argName = arg.name;
                            var argType = Context.followWithAbstracts(Context.getType(haxe.macro.ComplexTypeTools.toString(arg.type)));
                            
                            switch (argType) {
                                case TInst(t, _) if (t.get().name == "String"):
                                    callArgs.push(macro (cast $i{argName} : cpp.ConstCharStar));
                                    cppArgTypes.push("const char*");
                                case TAbstract(t, _) if (t.get().name == "Bool"):
                                    callArgs.push(macro ($i{argName} ? 1 : 0));
                                    cppArgTypes.push("int");
                                case TAbstract(t, _) if (t.get().name == "Int"):
                                    callArgs.push(macro $i{argName});
                                    cppArgTypes.push("int");
                                case TAbstract(t, _) if (t.get().name == "Float"):
                                    callArgs.push(macro $i{argName});
                                    cppArgTypes.push("double");
                                case TInst(t, _) if (t.get().meta.has(":ffi_struct")):
                                    callArgs.push(macro $i{argName}._getPointer());
                                    cppArgTypes.push("void*");
                                default:
                                    callArgs.push(macro $i{argName});
                                    cppArgTypes.push("void*");
                            }
                        }

                        var retType = Context.followWithAbstracts(Context.getType(haxe.macro.ComplexTypeTools.toString(f.ret)));
                        var cppRetType = "void*";
                        var isRetString = false;
                        var isRetBool = false;

                        switch (retType) {
                            case TInst(t, _) if (t.get().name == "String"): 
                                isRetString = true;
                                cppRetType = "const char*";
                            case TAbstract(t, _) if (t.get().name == "Bool"): 
                                isRetBool = true;
                                cppRetType = "int";
                            case TAbstract(t, _) if (t.get().name == "Int"):
                                cppRetType = "int";
                            case TAbstract(t, _) if (t.get().name == "Float"):
                                cppRetType = "double";
                            default:
                                if (typeName(retType) == "Void") cppRetType = "void";
                        }

                        var argSignature = cppArgTypes.join(",");
                        var placeholders = [for (i in 0...callArgs.length) "{" + (i + 1) + "}"].join(",");
                        var castStr = "((" + cppRetType + " (*)(" + argSignature + ")) {0})(" + placeholders + ")";
                        
                        var blockExprs = [
                            macro {
                                if ($i{wrapperName} == 0) {
                                    $i{wrapperName} = digigun.sys.dl.Dl.getSymbol(new digigun.sys.NativeHandle($i{libVarName}), $v{nativeName}).value;
                                }
                                if ($i{wrapperName} == 0) throw "FFI: Symbol " + $v{nativeName} + " not found";
                            }
                        ];

                        var untypedCall;
                        if (callArgs.length == 0) {
                            untypedCall = macro untyped __cpp__($v{castStr}, (untyped $i{wrapperName}));
                        } else if (callArgs.length == 1) {
                            untypedCall = macro untyped __cpp__($v{castStr}, (untyped $i{wrapperName}), ${callArgs[0]});
                        } else if (callArgs.length == 2) {
                            untypedCall = macro untyped __cpp__($v{castStr}, (untyped $i{wrapperName}), ${callArgs[0]}, ${callArgs[1]});
                        } else {
                            untypedCall = macro untyped __cpp__($v{castStr}, (untyped $i{wrapperName}), $a{callArgs});
                        }

                        if (isRetString) {
                            blockExprs.push(macro {
                                var res:cpp.ConstCharStar = $untypedCall;
                                return (res == null) ? null : res.toString();
                            });
                        } else if (isRetBool) {
                            blockExprs.push(macro {
                                var res:Int = $untypedCall;
                                return res != 0;
                            });
                        } else {
                            blockExprs.push(macro {
                                return $untypedCall;
                            });
                        }

                        field.kind = FFun({
                            args: f.args,
                            ret: f.ret,
                            expr: macro $b{blockExprs}
                        });
                        newFields.push(field);
                    default:
                        newFields.push(field);
                }
            } else {
                newFields.push(field);
            }
        }

        newFields.push({
            name: libVarName,
            access: [APrivate, AStatic],
            kind: FVar(macro : haxe.Int64, macro (0 : haxe.Int64)),
            pos: Context.currentPos()
        });

        newFields.push({
            name: "bind",
            access: [APublic, AStatic],
            kind: FFun({
                args: [{name: "path", type: macro : String}],
                ret: macro : Bool,
                expr: macro {
                    var h = digigun.sys.dl.Dl.open(path);
                    $i{libVarName} = h.value;
                    return h.isValid;
                }
            }),
            pos: Context.currentPos()
        });

        return newFields;
    }

    private static function typeName(t:Type):String {
        return switch(t) {
            case TAbstract(tr, _): tr.get().name;
            default: "";
        }
    }
}
