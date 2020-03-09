// gmmbin/gmm-global-est-gaussians-ebw.cc

// Copyright 2009-2011  Petr Motlicek  Chao Weng
//           2019       xmuspeech  ZM

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "gmm/diag-gmm.h"
#include "gmm/ebw-diag-gmm.h"

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;

    const char *usage =
        "Do EBW update for MMI, MPE or MCE discriminative training.\n"
        "Numerator stats should already be I-smoothed (e.g. use gmm-global-ismooth-stats)\n"
        "Usage:  gmm-global-est-gaussians-ebw [options] <model-in> <stats-num-in> <stats-den-in> <model-out>\n"
        "e.g.: gmm-global-est-gaussians-ebw 1.gmm num.acc den.acc 2.gmm\n";

    bool binary_write = false;
    int32 mixup = 0;
    BaseFloat perturb_factor = 0.01;
    std::string update_flags_str = "mv";

    EbwOptions ebw_opts;
    ParseOptions po(usage);
    po.Register("binary", &binary_write, "Write output in binary mode");
    po.Register("update-flags", &update_flags_str, "Which GMM parameters to "
                "update: e.g. m or mv (w, t ignored).");
    po.Register("mix-up", &mixup, "Increase number of mixture components to "
        "this overall target.");
    po.Register("perturb-factor", &perturb_factor, "While mixing up, perturb "
        "means by standard deviation times this factor.");
    ebw_opts.Register(&po);

    po.Read(argc, argv);

    if (po.NumArgs() != 4) {
      po.PrintUsage();
      exit(1);
    }

    kaldi::GmmFlagsType update_flags =
        StringToGmmFlags(update_flags_str);    

    std::string model_in_filename = po.GetArg(1),
        num_stats_filename = po.GetArg(2),
        den_stats_filename = po.GetArg(3),
        model_out_filename = po.GetArg(4);

    DiagGmm gmm;
    {
        bool binary_read;
        Input ki(model_in_filename, &binary_read);
        gmm.Read(ki.Stream(), binary_read);
    }

    AccumDiagGmm num_stats;
    AccumDiagGmm den_stats;
    {
      bool binary;
      Input ki(num_stats_filename, &binary);
      num_stats.Read(ki.Stream(), binary, true);  // true == add; doesn't matter here.
    }
    
    {
      bool binary;
      Input ki(den_stats_filename, &binary);
      den_stats.Read(ki.Stream(), binary, true);  // true == add; doesn't matter here.
    }
      
 
    {  // Update GMMs.
      BaseFloat auxf_impr, count;
      int32 num_floored;
      UpdateEbwDiagGmm(num_stats, den_stats, update_flags, ebw_opts, &gmm,
                          &auxf_impr, &count, &num_floored);
     
      KALDI_LOG << "Overall auxf impr/frame from Gaussian update is " << (auxf_impr/count)
                << " over " << count << " frames; floored D for "
                << num_floored << " Gaussians.";
    }

    if (mixup != 0)
        gmm.Split(mixup, perturb_factor);

    WriteKaldiObject(gmm, model_out_filename, binary_write);

    KALDI_LOG << "Written model to " << model_out_filename;

  } catch(const std::exception &e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
}
 