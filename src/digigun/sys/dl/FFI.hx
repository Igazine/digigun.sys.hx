package digigun.sys.dl;

import haxe.macro.Context;
import haxe.macro.Expr;
import haxe.macro.Type;

/**
 * Advanced FFI automation for Dynamic Symbol Loading.
 */
class FFI {
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
}
