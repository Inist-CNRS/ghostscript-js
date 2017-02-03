#include <nan.h>
#include <chrono>
#include <thread>
#include "ghostpdl/base/gserrors.h"
#include "ghostpdl/psi/iapi.h"

using namespace std;
using namespace Nan;
using namespace v8;

extern "C" {
  int gsapi_new_instance (void **pinstance, void *caller_handle);
  void gsapi_delete_instance (void *instance);
  int gsapi_set_arg_encoding (void *instance, int encoding);
  int gsapi_init_with_args (void *instance, int argc, char **argv);
  int gsapi_exit (void *instance);
};

class Ghostscript : public AsyncWorker {
  public: Ghostscript(Callback * callback) : AsyncWorker(callback) {}

  // Executes in worker thread
  void Execute() {
    // std::this_thread::sleep_for(chrono::milliseconds(1000));
    gsargv[0] = (char *) "gs";
    gsargv[1] = (char *) "-q";
    gsargv[2] = (char *) "-dNOPAUSE";
    gsargv[3] = (char *) "-dBATCH";
    gsargv[4] = (char *) "-sDEVICE=tiff24nc";
    gsargv[5] = (char *) "-r300";
    gsargv[6] = (char *) "-sOutputFile=test-%03d.tiff";
    gsargv[7] = (char *) "test.pdf";
    gsargc = 8;

    code = gsapi_new_instance(&minst, NULL);
    // // std::printf("code : %d\n", code);
    // // std::printf("argc : %d\n", argc);
    // // std::printf("argv[1] : %s\n", argv[1]);
    //
    // if (code < 0)
    //     return 1;
    code = gsapi_set_arg_encoding(minst, GS_ARG_ENCODING_UTF8);

    if (code == 0)
        code = gsapi_init_with_args(minst, gsargc, gsargv);
    code1 = gsapi_exit(minst);

    if ((code == 0) || (code == gs_error_Quit))
        code = code1;

    gsapi_delete_instance(minst);
  }
  // Executes in event loop
  void HandleOKCallback () {
    if ((code == 0) || (code == gs_error_Quit)) {
      Local<String> allOK = Nan::New<String>("All is ok !").ToLocalChecked();
      Local<Value> argv[] = { allOK };
      callback->Call(2, argv);
    } else {
      Local<String> error = Nan::New<String>("Something is wrong, dude !").ToLocalChecked();
      Local<Value> argv[] = { error };
      callback->Call(1, argv);
    }
  }

  private:
    int code, code1;
    char *gsargv[8];
    int gsargc;
    void *minst;
};

NAN_METHOD(exec) {
  Callback *callback = new Callback(info[0].As<Function>());
  AsyncQueueWorker(new Ghostscript(callback));
}

NAN_MODULE_INIT(Init) {
  Nan::Set(target, New<String>("exec").ToLocalChecked(),
    GetFunction(New<FunctionTemplate>(exec)).ToLocalChecked());
}

NODE_MODULE(addon, Init)
