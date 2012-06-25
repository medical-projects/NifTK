#!/bin/bash

#/*================================================================================
#
#  NifTK: An image processing toolkit jointly developed by the
#              Dementia Research Centre, and the Centre For Medical Image Computing
#              at University College London.
#  
#  See:        http://dementia.ion.ucl.ac.uk/
#              http://cmic.cs.ucl.ac.uk/
#              http://www.ucl.ac.uk/
#
#  Copyright (c) UCL : See LICENSE.txt in the top level directory for details. 
#
#  Last Changed      : $LastChangedDate: 2011-02-22 10:40:40 +0000 (Tue, 22 Feb 2011) $ 
#  Revision          : $Revision: 5284 $
#  Last modified by  : $Author: kkl $
#
#  Original author   : leung@drc.ion.ucl.ac.uk
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the above copyright notices for more information.
#
#=================================================================================*/

function Usage()
{
cat <<EOF

This script is the working script which is called ComputePairwiseRegistrationBatch.sh to perform pairwise registration. Please use ComputePairwiseRegistrationBatch.sh. 
  
EOF
exit 127
}

if [ $# -lt 3 ]
then 
  Usage
fi 



set -x

current_dir=`dirname $0`

output_format=$1
dilation=$2
symmetric=$3
dof=$4
scaling_using_skull=$5
similarity=$6
ajc=$7
pptol=$8
region=${9}
starting_arg=10

if [ "${MY_TEMP}" == "" ]
then
  temp_dir_prefix=~/temp/
else
  temp_dir_prefix=${MY_TEMP}
fi   

tmp_dir=`mktemp -d -q ~/temp/__affine_groupwise_reg.XXXXXX`
function cleanup
{
  echo "Cleaning up..."
  rm -rf  ${tmp_dir}
}
trap "cleanup" EXIT SIGINT SIGTERM SIGKILL 


dilation_flag=""
symmetric_flag="-sym_midway"
number_of_images=1
ajc_flag=""
symmetric_ajc_flag="0"

if [ ${dilation} \> 0 ] 
then 
  dilation_flag="-d ${dilation}"
fi 

if [ "${symmetric}" == "no" ]
then
  symmetric_flag=""
else
  symmetric_flag=-"${symmetric}"
fi 

if [ "${ajc}" == "yes" ]
then 
  ajc_flag="-ajc"
  symmetric_ajc_flag="1"
fi 


# Perform all pairwise registration. 
for (( arg=${starting_arg}; arg<=$#; arg+=4 ))
do
  for (( inner_arg=${starting_arg}; inner_arg<=$#; inner_arg+=4 ))
  do 
    if [ ${arg} == ${inner_arg} ] 
    then 
      continue
    fi 
  
    (( i=(arg-starting_arg)/4 ))
    (( j=(inner_arg-starting_arg)/4 ))
    output=`printf ${output_format} ${i} ${j}`
    
    (( fixed_img_arg=arg ))
    fixed_image=${!fixed_img_arg}
    (( fixed_mask_arg=arg+1 ))
    fixed_image_mask=${!fixed_mask_arg}
    (( fixed_roi1_arg=arg+2 ))
    fixed_roi1=${!fixed_roi1_arg}
    (( fixed_roi2_arg=arg+3 ))
    fixed_roi2=${!fixed_roi2_arg}
    
    (( img_arg=inner_arg ))
    moving_image=${!img_arg}
    (( mask_arg=inner_arg+1 ))
    moving_image_mask=${!mask_arg}
    (( roi1_arg=inner_arg+2 ))
    moving_roi1=${!roi1_arg}
    (( roi2_arg=inner_arg+3 ))
    moving_roi2=${!roi2_arg}
    
    if [ "${fixed_image}" == "dummy" ] || [ "${moving_image}" == "dummy" ]
    then 
      continue
    fi 
  
    if [ 1 ]
    then 
  
    if [ "${region}" == "yes" ] 
    then 
      fixed_image_mask_img=${tmp_dir}/`basename ${fixed_image_mask}_fixed.img`
      makemask ${fixed_image} ${fixed_image_mask} ${fixed_image_mask_img}
    
      moving_image_mask_img=${tmp_dir}/`basename ${moving_image_mask}_moving.img`
      makemask ${moving_image} ${moving_image_mask} ${moving_image_mask_img}
  
      if [ ! -f "${output}_affine_init.dof" ]
      then 
        # Initial affine registration to align using the region. 
        niftkAffine \
          -ti ${fixed_image} -si ${moving_image} \
          -tm ${fixed_image_mask_img} -sm ${moving_image_mask_img} \
          -ot ${output}_affine_init.dof \
          -ri 2 -fi 3 -s 4 -tr 2 -o 7 -ln 4 -rmin 0.1 -rmax 2 -stl 0 -spl 2 ${symmetric_flag} ${dilation_flag} -wsim 2 -pptol 0.001 -cog
      fi 
      
      if [ ! -f "${output}_affine_first.dof" ]
      then 
        # More precise affine registration. 
        niftkAffine \
          -ti ${fixed_image} -si ${moving_image} \
          -tm ${fixed_image_mask_img} \
          -sm ${moving_image_mask_img} \
          -ot ${output}_affine_first.dof \
          -it ${output}_affine_init.dof \
          -ri 2 -fi 3 -s ${similarity} -tr ${dof} -o 7 -ln 1 -rmin 0.001 -rmax 1 ${symmetric_flag} ${dilation_flag} -wsim 2 -pptol ${pptol}
      fi 
    
    else # if [ "${region}" == "no" ] 
  
      if [ ! -f "${output}_affine_first.dof" ]
      then 
        niftkAffine \
          -ti ${fixed_image} -si ${moving_image} \
          -ot ${output}_affine_first.dof \
          -ri 2 -fi 3 -s ${similarity} -tr ${dof} -o 6 -ln 3 -rmax 1 -rmin 0.01 ${symmetric_flag} -wsim 2 -pptol 0.001
      fi 
        
    fi 
    
    if [ ! -f "${output}_affine_second.dof" ]
    then 
      niftkAffine \
        -ti ${fixed_image} -si ${moving_image} \
        -tm ${fixed_image_mask_img} \
        -sm ${moving_image_mask_img} \
        -ot ${output}_affine_second.dof \
        -it ${output}_affine_first.dof \
        -ri 2 -fi 3 -s ${similarity} -tr 2 -o 7 -ln 1 -rmin 0.001 -rmax 0.5 ${symmetric_flag} -d 2 -wsim 2 -pptol ${pptol}
    fi 
    
    # ROI 1 local registration. 
    if [ ! -f "${output}_affine_second_roi1.dof" ] && [ -f "${fixed_roi1}" ] && [ -f "${moving_roi1}" ]
    then 
      if [ "${fixed_roi1%.img}" != "${fixed_roi1}" ] 
      then 
        fixed_roi1_img=${fixed_roi1}
      else
        fixed_roi1_img=${tmp_dir}/fixed_roi1.img
        makemask ${fixed_image} ${fixed_roi1} ${fixed_roi1_img}
      fi 
      if [ "${moving_roi1%.img}" != "${moving_roi1}" ]
      then
        moving_roi1_img=${moving_roi1}
      else
        moving_roi1_img=${tmp_dir}/moving_roi1.img
        makemask ${moving_image} ${moving_roi1} ${moving_roi1_img}
      fi 
      niftkAffine \
        -ti ${fixed_image} -si ${moving_image} \
        -tm ${fixed_roi1_img} \
        -sm ${moving_roi1_img} \
        -ot ${output}_affine_second_roi1.dof \
        -it ${output}_affine_second.dof \
        -ri 2 -fi 3 -s ${similarity} -tr 2 -o 7 -ln 1 -rmin 0.001 -rmax 0.5 ${symmetric_flag} -d 5 -wsim 2 -pptol ${pptol}
    fi 
    
    # ROI 2 local registration. 
    if [ ! -f "${output}_affine_second_roi2.dof" ] && [ -f "${fixed_roi2}" ] && [ -f "${moving_roi2}" ]
    then 
      if [ "${fixed_roi2%.img}" != "${fixed_roi2}" ]
      then
        fixed_roi2_img=${fixed_roi2}
      else
        fixed_roi2_img=${tmp_dir}/fixed_roi2.img
        makemask ${fixed_image} ${fixed_roi2} ${fixed_roi2_img}
      fi
      if [ "${moving_roi2%.img}" != "${moving_roi2}" ]
      then 
        moving_roi2_img=${moving_roi2}
      else
       moving_roi2_img=${tmp_dir}/moving_roi2.img
       makemask ${moving_image} ${moving_roi2} ${moving_roi2_img}
      fi 
      niftkAffine \
        -ti ${fixed_image} -si ${moving_image} \
        -tm ${fixed_roi2_img} \
        -sm ${moving_roi2_img} \
        -ot ${output}_affine_second_roi2.dof \
        -it ${output}_affine_second.dof \
        -ri 2 -fi 3 -s ${similarity} -tr 2 -o 7 -ln 1 -rmin 0.001 -rmax 0.5 ${symmetric_flag} -d 5 -wsim 2 -pptol ${pptol}
    fi 
    
    fi 
     
    (( number_of_images++ ))
  done
  
done

rm -rf ${tmp_dir}





