FROM ubuntu
RUN apt-get update
RUN apt-get install -y autoconf flex bison make wget clang g++
RUN apt-get install -y perl-modules
RUN apt-get install -y libfmt-dev
RUN bash -c "wget https://www.veripool.org/ftp/verilator-4.010.tgz; tar xvzf verilator*.t*gz; ls; cd verilator-4.010; ./configure --prefix=/usr/; make -j $(nproc); make install"
RUN clang --version
RUN gcc --version
RUN verilator --version
