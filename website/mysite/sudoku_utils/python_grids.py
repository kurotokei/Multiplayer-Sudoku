"""
this module will contain the python function to get a puzzle
randomize it , and manipulate th current state of the grid.
"""

import ctypes,random,os,threading


libc = ctypes.CDLL('libc.so.6')

libc.free.restype  = None
libc.free.argtypes = [ ctypes.c_void_p ]


base_dir = os.path.dirname(os.path.abspath(__file__))

data_file_name = os.path.join(base_dir, "sudoku_puzzles.data")

data_file = open(data_file_name, "rb")

data_file_lock = threading.Lock()

sudoku = ctypes.CDLL(os.path.join(base_dir, "sudoku.so"))

class C__full_grid_t(ctypes.Structure):
    _fields_ = [ ("squares", ctypes.c_uint8 * 9 * 9) ]

( GRID_VALUE_ORIGIN_START,
  GRID_VALUE_ORIGIN_PLAYER_1,
  GRID_VALUE_ORIGIN_PLAYER_2 ) = map( ctypes.c_int, range(3) )

class C__current_position_t(ctypes.Structure):
    _fields_ = [
                ("solution", C__full_grid_t),
                ("position", C__full_grid_t),
                ("origins",  ctypes.c_int * 9 * 9)
                ]

# hard code the value beacause using
#   ctypes.c_int.in_dll(sudoku, "NB_LEVELS")
# does not work unless we define both
# a macro AND a global variables with the
# same value
NB_LEVELS = 3

class C__puzzle_t(ctypes.Structure):
    _fields_ = [
                ("solution", C__full_grid_t),
                ("levels", C__full_grid_t * NB_LEVELS)
                ]


sudoku.randomize_puzzle.restype  = None
sudoku.randomize_puzzle.argtypes = [ 
                                    ctypes.POINTER(C__puzzle_t),
                                    ctypes.c_int,
                                    ctypes.c_int
                                    ]

sudoku.puzzle_to_current_position.restype  = C__current_position_t
sudoku.puzzle_to_current_position.argtypes = [
                                        ctypes.POINTER(C__puzzle_t),
                                        ctypes.c_int
                                    ]

sudoku.current_position_to_json_string.restype  = ctypes.c_void_p
sudoku.current_position_to_json_string.argtypes = [ 
                                        ctypes.POINTER(C__current_position_t),
                                        ctypes.c_bool
                                        ]


nb_puzzles = os.path.getsize(data_file_name) // ctypes.sizeof(C__puzzle_t)

def get_new_game_position(level):
    """
        choose a random puzzle
        randomize it
        return current_position_t from it
    """

    if type(level) != int  or  not 0 <= level < NB_LEVELS :
        raise ValueError("given level is not valid")

    with data_file_lock:
        data_file.seek( ctypes.sizeof(C__puzzle_t) *
                        random.randrange(nb_puzzles))

        puzzle_bytes = data_file.read(ctypes.sizeof(C__puzzle_t))


    puzzle = C__puzzle_t()

    ctypes.memmove( ctypes.byref(puzzle),
                    puzzle_bytes,
                    ctypes.sizeof(C__puzzle_t))

    sudoku.randomize_puzzle( ctypes.byref(puzzle),
                             level,
                             random.randrange(1<<32) )

    return sudoku.puzzle_to_current_position( ctypes.byref(puzzle),
                                              level)

def current_position_to_json_string(current_position_ptr, include_solution):
    """this is a wrapper for the c++ function, to avoid memory leaks"""

    # c_str is a void*
    c_str = sudoku.current_position_to_json_string(
                                    current_position_ptr,
                                    ctypes.c_bool(include_solution))

    python_str = ctypes.cast(c_str, ctypes.c_char_p).value

    # free the memory
    libc.free(c_str)

    return python_str

def bytes_to_current_position(position_bytes):
    """used to convert the bytes in the DB to a ctypes object"""

    position = C__current_position_t()
    ctypes.memmove( ctypes.byref(position),
                    position_bytes,
                    ctypes.sizeof(C__current_position_t))

    return position
