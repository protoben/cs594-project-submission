CS594 Project Submission
========================

My project was to implement a secure version of the MORE protocol using random
matrix checksums. I got as far as implementing RLNC, but fell short of adding
checksums or making nodes do forwarding.

I implemented my project in ns-3, so I'm including the entire ns-3 software to
make it easy to compile. I've tested compilation on ada.cs.pdx.edu. It should
be enough to `cd` into the repository directory and type

    make && make run

to run the basic simulation. If you want to change options around (for example,
to change the distance between nodes on the grid to 600m) you'll need to do
the following from the repository directory:

    cd ns-3.24
    ./waf --run scratch/smore --command-template='%s --distance=600'

To see a list of options, try

    ./waf --run scratch/smore --command-template='%s --distance=600'

If you'd like to look at the source files I created, they're all contained in
the `files` subdirectory of the repository directory, for your convenience.

My RFC is in the file called smore-rfc.txt in the repository directory. If you
want to look at the nroff source (I'm not sure why you would...), that's the
corresponding `.roff` file.
