//
// Created by kannabi on 30.09.16.
//

#include "header.h"

int main (){

    char ans;

    printf ("Do u wanna start server (enter 's') r client (enter 'c')");

    scanf ("%c", &ans);

    switch (ans){
        case 's': {server();}
        case 'c': {client();}
    }

    return EXIT_SUCCESS;
}