#/*============================================================================
#
#  NifTK: A software platform for medical image computing.
#
#  Copyright (c) University College London (UCL). All rights reserved.
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  See LICENSE.txt in the top level directory for details.
#
#============================================================================*/

echo "Helper functions for brain to brain registration" 

function brain_to_brain_registration_without_repeat_mask_using_air()
{
  local baseline_image=$1
  local baseline_region=$2
  local repeat_image=$3
  local output_reg_brain_image=$4
  local output_reg_brain_series_number=$5
  local output_reg_air=$6
  local is_invert_air=$7

  local tmp_dir=`mktemp -d -q ~/temp/hippo-template-match-reg.XXXXXX`
  
  local whole_brain_initfile=${tmp_dir}/whole_brain_reg.ini
  makemask ${baseline_image} ${baseline_region} ${tmp_dir}/baseline_brain_mask -d 2
  t1=`imginfo ${baseline_image} -tanz 0.2`
  t2=`imginfo ${repeat_image} -tanz 0.2`
  ${AIR_BIN}/alignlinear ${baseline_image} ${repeat_image} ${output_reg_air} -g ${whole_brain_initfile} y -m 12 \
     -e1 ${tmp_dir}/baseline_brain_mask -p1 1 -p2 1 -s 81 1 3 -c 0.0001 -h 200 -r 200 -q -x 1 \
     -t1 $t1 -t2 $t2 -v -b1 2 2 2 -b2 2 2 2
  rm -f ${output_reg_air}
  ${AIR_BIN}/alignlinear ${baseline_image} ${repeat_image} ${output_reg_air} -m 12 \
     -e1 ${tmp_dir}/baseline_brain_mask \
     -f ${whole_brain_initfile} -p1 1 -p2 1 -s 2 1 2 -c 0.0001 -h 200 -r 200 -q -x 1 -t1 $t1 -t2 $t2 -v 
     
  if [ "${is_invert_air}" == "no" ]
  then 
    ${AIR_BIN}/reslice ${output_reg_air} ${output_reg_brain_image} -k -n 10 -o
  else
    ${AIR_BIN}/invert_air ${output_reg_air} ${output_reg_air} y
    ${AIR_BIN}/reslice ${output_reg_air} ${output_reg_brain_image} -k -n 10 -o
  fi 
  rm -rf ${tmp_dir}
  
  return 0
}

function brain_to_brain_registration_without_repeat_mask_using_irtk()
{
  local baseline_image=$1
  local baseline_region=$2
  local repeat_image=$3
  local output_reg_brain_image=$4
  local output_reg_brain_series_number=$5
  local output_reg_air=${6}
  local is_invert_air=$7
  local is_9dof=$8
  local is_blurring=$9

  local temp_dir=`mktemp -d -q ~/temp/__areg.XXXXXXXX`
  set -x

  if [ ! -f ${output_reg_air} ] 
  then 
  
    if [ "${is_9dof}" == "yes" ] 
    then 
      # Registration. 
      local parameter_file=`mktemp ~/temp/param.XXXXXXXXXX`
      if [ "${is_blurring}" == "yes" ]
      then
        echo "Target blurring (in mm) = 1 "  > ${parameter_file}
        echo "Target resolution (in mm) = 1"  >> ${parameter_file}
        echo "# source image paramters"  >> ${parameter_file}
        echo "Source blurring (in mm) = 1"  >> ${parameter_file}
        echo "Source resolution (in mm)  = 1"  >> ${parameter_file}
        echo "# registration parameters"  >> ${parameter_file}
        echo "No. of resolution levels = 3"  >> ${parameter_file}
        echo "No. of bins = 128"  >> ${parameter_file}
        echo "No. of iterations = 100"  >> ${parameter_file}
        echo "No. of steps = 2"  >> ${parameter_file}
        echo "Length of steps = 1"  >> ${parameter_file}
        echo "Similarity measure = CC"  >> ${parameter_file}
      else
        echo "Target blurring (in mm) = 0 "  > ${parameter_file}
        echo "Target resolution (in mm) = 0"  >> ${parameter_file}
        echo "# source image paramters"  >> ${parameter_file}
        echo "Source blurring (in mm) = 0"  >> ${parameter_file}
        echo "Source resolution (in mm)  = 0"  >> ${parameter_file}
        echo "# registration parameters"  >> ${parameter_file}
        echo "No. of resolution levels = 3"  >> ${parameter_file}
        echo "No. of bins = 64"  >> ${parameter_file}
        echo "No. of iterations = 100"  >> ${parameter_file}
        echo "No. of steps = 2"  >> ${parameter_file}
        echo "Length of steps = 1"  >> ${parameter_file}
        echo "Similarity measure = NMI"  >> ${parameter_file}
      fi
      ${MIDAS_FFD}/ffdareg.sh ${baseline_image} ${repeat_image} ${output_reg_air} -dof 9 -comreg -params ${parameter_file} -tmpdir ${temp_dir} 
    else
      # Registration. 
      local parameter_file=`mktemp ~/temp/param.XXXXXXXXXX`
      echo "Target blurring (in mm) = 0 "  > ${parameter_file}
      echo "Target resolution (in mm) = 0"  >> ${parameter_file}
      echo "# source image paramters"  >> ${parameter_file}
      echo "Source blurring (in mm) = 0"  >> ${parameter_file}
      echo "Source resolution (in mm)  = 0"  >> ${parameter_file}
      echo "# registration parameters"  >> ${parameter_file}
      echo "No. of resolution levels = 3"  >> ${parameter_file}
      echo "No. of bins = 128"  >> ${parameter_file}
      echo "No. of iterations = 100"  >> ${parameter_file}
      echo "No. of steps = 2"  >> ${parameter_file}
      echo "Length of steps = 1"  >> ${parameter_file}
      echo "Similarity measure = CC"  >> ${parameter_file}
      ${MIDAS_FFD}/ffdareg.sh ${baseline_image} ${repeat_image} ${output_reg_air} -dof 12 -comreg -params ${parameter_file} -tmpdir ${temp_dir} 
    fi 
    rm -f ${parameter_file}
    
    
    local temp_dir1=`mktemp -d -q ~/temp/__areg.XXXXXXXX`
    
    makemask ${baseline_image} ${baseline_region} ${temp_dir1}/baseline_region.img -d 8 
    makeroi -img ${temp_dir1}/baseline_region.img -out ${temp_dir1}/baseline_region -alt 0 
    if [ "${is_9dof}" == "yes" ] 
    then 
      local parameter_file=`mktemp ~/temp/param.XXXXXXXXXX`
      if [ "${is_blurring}" == "yes" ]
      then
        echo "Target blurring (in mm) = 0 "  > ${parameter_file}
        echo "Target resolution (in mm) = 0"  >> ${parameter_file}
        echo "# source image paramters"  >> ${parameter_file}
        echo "Source blurring (in mm) = 0"  >> ${parameter_file}
        echo "Source resolution (in mm)  = 0"  >> ${parameter_file}
        echo "# registration parameters"  >> ${parameter_file}
        echo "No. of resolution levels = 2"  >> ${parameter_file}
        echo "No. of bins = 128"  >> ${parameter_file}
        echo "No. of iterations = 100"  >> ${parameter_file}
        echo "No. of steps = 4"  >> ${parameter_file}
        echo "Length of steps = 1"  >> ${parameter_file}
        echo "Similarity measure = CC"  >> ${parameter_file}
      else
        echo "Target blurring (in mm) = 0 "  > ${parameter_file}
        echo "Target resolution (in mm) = 0"  >> ${parameter_file}
        echo "# source image paramters"  >> ${parameter_file}
        echo "Source blurring (in mm) = 0"  >> ${parameter_file}
        echo "Source resolution (in mm)  = 0"  >> ${parameter_file}
        echo "# registration parameters"  >> ${parameter_file}
        echo "No. of resolution levels = 2"  >> ${parameter_file}
        echo "No. of bins = 64"  >> ${parameter_file}
        echo "No. of iterations = 100"  >> ${parameter_file}
        echo "No. of steps = 4"  >> ${parameter_file}
        echo "Length of steps = 1"  >> ${parameter_file}
        echo "Similarity measure = CC"  >> ${parameter_file}
      fi
      ${MIDAS_FFD}/ffdareg.sh ${baseline_image} ${repeat_image} ${output_reg_air} -troi ${temp_dir1}/baseline_region -dof 9 -inidof ${output_reg_air} -params ${parameter_file} -tmpdir ${temp_dir}
    else
       local parameter_file=`mktemp ~/temp/param.XXXXXXXXXX`
       echo "Target blurring (in mm) = 0 "  > ${parameter_file}
       echo "Target resolution (in mm) = 0"  >> ${parameter_file}
       echo "# source image paramters"  >> ${parameter_file}
       echo "Source blurring (in mm) = 0"  >> ${parameter_file}
       echo "Source resolution (in mm)  = 0"  >> ${parameter_file}
       echo "# registration parameters"  >> ${parameter_file}
       echo "No. of resolution levels = 1"  >> ${parameter_file}
       echo "No. of bins = 128"  >> ${parameter_file}
       echo "No. of iterations = 100"  >> ${parameter_file}
       echo "No. of steps = 4"  >> ${parameter_file}
       echo "Length of steps = 2"  >> ${parameter_file}
       echo "Similarity measure = CC"  >> ${parameter_file}
       ${MIDAS_FFD}/ffdareg.sh ${baseline_image} ${repeat_image} ${output_reg_air} -troi ${baseline_region} -dof 12 -inidof ${output_reg_air} -params ${parameter_file} -tmpdir ${temp_dir}    
    fi 
    rm -f ${parameter_file}
    rm -rf ${temp_dir1}
  fi 
  
  # Transform the image. 
  if [ "${is_invert_air}" == "yes" ]
  then 
    local invert_flag="-invert"
    #${MIDAS_FFD}/ffdtransformation.sh  ${baseline_image} ${repeat_image} ${output_reg_brain_image} ${output_reg_air} -cspline -invert -tmpdir ${temp_dir}
    ${MIDAS_FFD}/ffdtransformation.sh  ${baseline_image} ${repeat_image} ${output_reg_brain_image} ${output_reg_air} -sinc -invert -tmpdir ${temp_dir}
    local orientation=`imginfo ${repeat_image} -orient`
    anchange ${output_reg_brain_image} ${output_reg_brain_image} -setorient ${orientation}
  else
    ${MIDAS_FFD}/ffdtransformation.sh ${repeat_image} ${baseline_image} ${output_reg_brain_image} ${output_reg_air} -cspline -tmpdir ${temp_dir}
    local orientation=`imginfo ${baseline_image} -orient`
    anchange ${output_reg_brain_image} ${output_reg_brain_image} -setorient ${orientation}
  fi 
  
  rm ${temp_dir} -rf 
  
  return 0
}



