import sys


#read arguments
program_filepath = sys.argv[1]


#Tokenize

program_lines = []
with open(program_filepath, "r") as program_file:
    program_lines = [line.strip() for line in program_file.readlines()]


#Tokenize
#[Keyword]
#[Variable]
#[Operator]
#[Punctuation]
#[Literal]

program = []
token_counter = 0
label_tracker = {}

for line in program_lines:

    if "Inscribe" in line:
        Keyword = "Inscribe"
        Punctuaton = line[8]
        Literal = line.split("\"")
        print(Literal[1])
        
    else:
        parts = line.split(" ")
        Keyword = parts[0]

    print(Keyword)

