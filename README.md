# iaxmodem

This is a git-svn export of the Sourceforge iaxmodem code project.

Steps followed to create this repo:

    cat <<EOF > authors.txt
    faxguy = Lee Howard <faxguy@howardsilvan.com>
    fedeheinz = Federico Heinz <fheinz@gmail.com>
    EOF
    git svn clone --trunk=/ --authors-file=authors.txt https://svn.code.sf.net/p/iaxmodem/code/ iaxmodem
    git remote add origin git@github.com:ianblenke/iaxmodem.git
    git push -u origin master

The authors.txt file is also included in the same commit that added this README.md

