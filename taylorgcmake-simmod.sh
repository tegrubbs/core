#deps=/global/common/software/m5069/build_chef/chef_deps #/path/to/dependencies/install/dir
#sim=/global/common/software/m5069/build_chef/simmetrix/15.0-210213 #/path/to/simmetrix/simModSuite/install/dir
export SIMMETRIX_LIB_DIR=/home/tegrubbs/15.0-210213/lib/x86_64_linux-IB
export SIMMETRIX_INCLUDE_DIR=/home/tegrubbs/15.0-210213/include
export CMAKE_PREFIX_PATH=$SIMMETRIX_LIB_DIR:$SIMMETRIX_INCLUDE_DIR:$SIMMETRIX_LIB_DIR/psKrnl

PARMETIS_INSTALL_DIR=/home/tegrubbs/local/
ZOLTAN_INSTALL_DIR=/home/tegrubbs/local/
cmake .. \
  -DCMAKE_C_COMPILER="mpicc" \
  -DCMAKE_CXX_COMPILER="mpicxx" \
  -DCMAKE_C_FLAGS=" -g -O3  -Wall -Wextra -Werror" \
  -DCMAKE_CXX_FLAGS=" -g -O3 -Wall -Wextra -Werror" \
  -DENABLE_ZOLTAN=ON \
  -DZOLTAN_INCLUDE_DIR="$ZOLTAN_INSTALL_DIR/include" \
  -DZOLTAN_LIBRARY="$ZOLTAN_INSTALL_DIR/lib/libzoltan.a" \
  -DMETIS_INCLUDE_DIR="$PARMETIS_INSTALL_DIR/include" \
  -DMETIS_LIBRARY="$PARMETIS_INSTALL_DIR/lib/libmetis.a" \
  -DPARMETIS_PREFIX="$PARMETIS_INSTALL_DIR" \
  -DPARMETIS_INCLUDE_DIR="$PARMETIS_INSTALL_DIR/include" \
  -DPARMETIS_LIBRARY="$PARMETIS_INSTALL_DIR/lib/libparmetis.a" \
  -DENABLE_SIMMETRIX=ON \
  -DSIM_PARASOLID=ON \
  -DCMAKE_EXE_LINKER_FLAGS="-ldl -ltirpc" \
  -DSIM_MPI=mpich3.1.2 \
  -DSIMMETRIX_LIB_DIR=$SIMMETRIX_LIB_DIR \
  -DSIMMETRIX_INCLUDE_DIR=$SIMMETRIX_INCLUDE_DIR \
  -DENABLE_OMEGA_H=OFF \
  -DSIM_ACIS=OFF \
  -DSCOREC_CXX_WARNINGS=ON \
  -DSCOREC_CXX_OPTIMIZE=OFF \
  -DUSE_XSDK_DEFAULTS=OFF \
  -DSCOREC_ENABLE_CXX11=ON \
  -DBUILD_EXES=ON \
  -DIS_TESTING=OFF \
  -DPCU_COMPRESS=True \
  -DMESHES="/home/tegrubbs/gmsh-chef/pumi-meshes/" 
  
