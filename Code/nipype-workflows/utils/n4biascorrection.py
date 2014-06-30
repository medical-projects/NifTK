# emacs: -*- mode: python; py-indent-offset: 4; indent-tabs-mode: nil -*-
# vi: set ft=python sts=4 ts=4 sw=4 et:
"""
    Simple interface for the CropImage.sh script
"""

import os

from nipype.interfaces.fsl.base import FSLCommand as N4BiasCorrectionCommand
from nipype.interfaces.fsl.base import FSLCommandInputSpec as N4BiasCorrectionCommandInputSpec
from nipype.interfaces.base import (TraitedSpec, File, Directory, traits, InputMultiPath,
                                    isdefined)

class N4BiasCorrectionInputSpec(N4BiasCorrectionCommandInputSpec):
    
    in_file = File(argstr="-i %s", exists=True, mandatory=True,
                   desc="Input target image filename")
    mask_file = File(exists=True, argstr="-m %s",
                     desc="Mask over the input image")
    out_file = File(argstr="-o %s", 
                    desc="output bias corrected image",
                    name_source = ['in_file'],
                    name_template = '%s_corrected')
    in_levels = traits.BaseInt(desc='Number of Multi-Scale Levels - optional - default = 3', 
                               argstr='-n %d')
    in_downsampling = traits.BaseInt(desc='Level of Downsampling - optional - default = 1 (no downsampling), downsampling to level 2 is recommended', 
                                     argstr='-d %d')
    in_maxiter = traits.BaseInt(desc='Maximum number of Iterations - optional - default = 50', 
                                argstr='-t %d')
    out_biasfield_file = File(argstr="-b %s", 
                              desc="output bias field file",
                              name_source = ['in_file'],
                              name_template = '%s_biasfield')
    in_convergence = traits.Float(desc='Convergence Threshold - optional - default = 0.001', 
                                  argstr='-c %f')


class N4BiasCorrectionOutputSpec(TraitedSpec):

    out_file = File(exists=True, desc="output bias corrected image")
    out_biasfield_file = File(desc="output bias field")

class N4BiasCorrection(N4BiasCorrectionCommand):

    _cmd = "n4biascorrection"

    input_spec = N4BiasCorrectionInputSpec  
    output_spec = N4BiasCorrectionOutputSpec

