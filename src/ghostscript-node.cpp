#include <nan.h>
#include <ghostscript/gserrors.h>
#include <ghostscript/iapi.h>

using namespace std;
using namespace Nan;
using namespace v8;

class ghostscript : public AsyncWorker {
public:
    ghostscript(Callback *callback, vector<string> args) : AsyncWorker(callback), args(args) {}

    // Executes in worker thread
    void Execute() {
        if (args.size() == 0) {
            gscode = 1;
            return;
        }

        void *minst;
        unsigned int gsargc = (unsigned int) args.size() + 1;
        char *gsargv[gsargc];
        gsargv[0] = (char *) "gs";
        for (int unsigned i = 0; i < args.size(); i++) {
            char *argsToChar = new char[args[i].length() + 1];
            strcpy(argsToChar, args[i].c_str());
            gsargv[i + 1] = argsToChar;
        }

        gscode = gsapi_new_instance(&minst, NULL);
        if (gscode < 0) return;
        gsapi_set_stdio(minst, gsdll_stdin, gsdll_stdout, gsdll_stderr);
        gscode = gsapi_set_arg_encoding(minst, GS_ARG_ENCODING_UTF8);

        if (gscode == 0) gscode = gsapi_init_with_args(minst, gsargc, gsargv);
        gscode1 = gsapi_exit(minst);

        if ((gscode == 0) || (gscode == gs_error_Quit))
            gscode = gscode1;

        gsapi_delete_instance(minst);

    }

    // Executes in event loop
    void HandleOKCallback() {
        if (gscode == gs_okay) {
            Local<Value> argv[] = {Nan::Null()};
            callback->Call(1, argv);
        } else {
            Local<Integer> codeError = Nan::New(gscode);
            Local<Value> argv[] = {codeError};
            callback->Call(1, argv);
        }
    }

private:
    vector<string> args;
    int gscode = 0;
    int gscode1;

    static int gsdll_stdin(void *instance, char *buf, int len) {
        int ch;
        int count = 0;
        while (count < len) {
            ch = fgetc(stdin);
            if (ch == EOF)
                return 0;
            *buf++ = (char) ch;
            count++;
            if (ch == '\n')
                break;
        }
        return count;
    }

    static int gsdll_stdout(void *instance, const char *str, int len) { return len; }

    static int gsdll_stderr(void *instance, const char *str, int len) { return len; }
};

NAN_METHOD(exec) {
    if (info.Length() < 2) {
        return Nan::ThrowError("Wrong number of arguments");
    }
    Local<Array> input = Local<Array>::Cast(info[0]);
    vector<string> args;
    if (input->IsArray()) {
        for (int unsigned i = 0; i < input->Length(); i++) {
            Nan::Utf8String val(Nan::Get(input, i).ToLocalChecked());
            std::string str(*val);
            args.push_back(str);
        }
    }
    Callback *callback = new Callback(info[1].As<Function>());
    AsyncQueueWorker(new ghostscript(callback, args));
}

NAN_MODULE_INIT(Init) {
    Nan::Set(target, New<String>("exec").ToLocalChecked(),
             GetFunction(New<FunctionTemplate>(exec)).ToLocalChecked());
}

NODE_MODULE(ghostscriptjs, Init)
