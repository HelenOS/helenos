# by Alf
# This script solves malfunction of symlinks in cygwin
# 
# Download sources from repository and than run this script to correct symlinks 
#   to be able compile project


if uname | grep 'CYGWIN' > /dev/null; then
  echo "Good ... you have cygwin"
else
  echo "Wrong. This script is only for cygwin"
  exit
fi 
 
for linkName in `find . ! -iwholename '.*svn*' ! -type d -print`; do
  if head -n 1 $linkName | grep '^link' > /dev/null; then
     linkTarget=`head -n 1 $linkName | sed 's/^link //'` 
     echo $linkName " -->" $linkTarget
     rm $linkName
     ln -s "$linkTarget" "$linkName" 
   fi   
done 




 

 
