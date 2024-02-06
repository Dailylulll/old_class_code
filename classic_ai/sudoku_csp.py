from __future__ import annotations  # for circular class dependencies

import time
from dataclasses import dataclass


@dataclass
class Element_count:
    element: int
    count = 0


@dataclass
class Element_sort:
    element: int
    index: int


# Universal coord list
@dataclass
class Spot_Ref:
    row: int
    col: int
    vectors: set[Vector_Ref]

    def __hash__(self):
        return hash(id(self))


@dataclass
class Vector_Ref:
    spot_list: set[Spot_Ref]

    def __hash__(self):
        return hash(id(self))


# possibilities for each spot
@dataclass
class Spot:
    domain: set[int]
    vec_membership: set[Vector]
    constraints: set[Spot]
    commit: int = 0
    r: int = 0
    c: int = 0

    def __hash__(self):
        return id(self)
    

# possibilities for each vector
@dataclass
class Vector:
    vec_membership: Vector_Ref
    domain: set[int]
    spot_members: set[Spot]

    def __hash__(self):
        return id(self)
    

# minimal info to keep track of iterations 
@dataclass
class Board:
    grid: list[list[Spot]]
    vec_list: set[Vector]
    queue: list[Spot]
    element_list: list[Element_count]()

    def __init__(self, square_num):
        self.grid = list[list[Spot]]()
        for r in range(square_num):
            self.grid.append(list[Spot]())
            for c in range(square_num):
                self.grid[r].append(Spot(set[int](), set[Vector](), set[Spot]()))
        self.vec_list = set[Vector]()
        self.queue = list[Spot]()
        self.element_list = list[Element_count]()

    def commit_move(self, new_move: tuple[int, int, int]):
        spot = self.grid[new_move[0]][new_move[1]]
        spot.commit = new_move[2]
        spot.domain = None

    def build_queue(self):
        for r in self.grid:
            for c in r:
                if c.commit != 0:
                    continue
                else:
                    self.queue.append(c)

    def sort_queue(self):
        self.queue.sort(key=lambda spot: (len(spot.domain), -len(spot.constraints), spot.c, spot.r))

    def pop_queue(self):
        return self.queue.pop(0)

    def check_queue(self):
        for i in self.queue:
            if len(i.domain) == 0:
                return False
            else:
                continue
        return True

    def check_win(self):
        check_win = True
        for r in self.grid:
            for c in r:
                if c.commit == 0:
                    check_win = False
        return check_win

    def print_board(self):
        square_num = len(self.grid)
        root_num = square_num ** .5
        dash_len = int(square_num + root_num - 1)
        for r in range(square_num):
            if r != 0 and r % root_num == 0:
                for _ in range(dash_len):
                    print("---", end="")
                print()
            for c in range(square_num):
                if c != 0 and c % root_num == 0:
                    print(" | ", end="")
                if self.grid[r][c].commit != 0:
                    print(f" {self.grid[r][c].commit} ", end="")
                else:
                    print("   ", end="")
            print()

    def print_board_domain(self):
        square_num = len(self.grid)
        root_num = square_num ** .5
        dash_len = int(square_num + root_num - 1)
        for r in range(square_num):
            if r != 0 and r % root_num == 0:
                for _ in range(dash_len):
                    print("----", end="")
                print()
            for c in range(square_num):
                if c != 0 and c % root_num == 0:
                    print(" | ", end="")
                if self.grid[r][c].domain is not None:
                    print(f"{len(self.grid[r][c].domain):^4}", end="")
                else:
                    print("    ", end="")
            print()
        pass

    def print_board_constraints(self):
        square_num = len(self.grid)
        root_num = square_num ** .5
        dash_len = int(square_num + root_num - 1)
        for r in range(square_num):
            if r != 0 and r % root_num == 0:
                for _ in range(dash_len):
                    print("----", end="")
                print()
            for c in range(square_num):
                if c != 0 and c % root_num == 0:
                    print(" | ", end="")
                if self.grid[r][c].commit == 0:
                    print(f"{len(self.grid[r][c].constraints):^4}", end="")
                else:
                    print("    ", end="")
            print()
        pass

    def print_spot(self, move: tuple[int, int, int]):
        spot = self.grid[move[0]][move[1]]
        if spot.domain is None:
            dl = ""
        else:
            dl = len(spot.domain)
        print(f"Coord: ({spot.r},{spot.c}) / Domain len: {dl} / Constraint len: {len(spot.constraints)}")


# functions to manipulate dataclasses
class Solver:
    root_num: int
    square_num: int
    grid_ref: list[list[Spot_Ref]]
    valid_moves: set[int] = set[int]()
    vec_set: set[Vector_Ref] = set[Vector_Ref]()
    start_grid: Board

    def __init__(self, root_num, start_info=None):
        self.root_num = root_num
        self.square_num = root_num ** 2
        self.grid_ref = self.build_grid()
        for i in range(1, self.square_num + 1):
            self.valid_moves.add(i)
        self.build_vector_ref()
        self.link_spot_to_vec()
        if start_info is not None:
            self.start_grid = self.init_grid(start_info)
        else:
            pass

    def __str__(self) -> str:
        string: str = ""
        for col in self.grid_ref:
            for spot in col:
                string += spot.__str__() + " "
            string += "\n"
        return string

    '''
    creating the very first board
    '''
    def init_grid(self, a: list[tuple[int, int, int]]) -> Board:
        new = Board(self.square_num)
        for i in a:
            spot = new.grid[i[0]][i[1]]
            spot.r = i[0]
            spot.c = i[1]
            spot.commit = i[2]
            if i[2] != 0:
                spot.domain = None
                continue
            else:
                spot.domain = self.valid_moves.copy()
        return new

    def process_board(self, board: Board, new_move: tuple[int, int, int] = None):
        if new_move is not None:
            board.commit_move(new_move)
        self.build_vector_sets(board)
        self.build_spot_domain(board)
        self.count_spot_constraints(board)
        self.count_domain_elements(board)
        board.build_queue()
        board.sort_queue()

    def check_constraints(self, board: Board):
        if board.check_queue() and self.arc_check(board):
            return True
        else:
            return False

    def count_domain_elements(self, board: Board):
        for i in self.valid_moves:
            board.element_list.append(Element_count(i))
        for r in range(self.square_num):
            for c in range(self.square_num):
                dspot = board.grid[r][c]
                if dspot.domain is None:
                    continue
                for number in dspot.domain:
                    for elem in board.element_list:
                        if elem.element == number:
                            elem.count += 1
        board.element_list.sort(key=lambda a: a.count)

    def arc_check(self, board: Board):
        # todo How to distrubute the set to avoid things like [(1), (1), (1, 2, 3)] this holds the rule
        # the union of all spot domains should be a superset of the vector domain
        # todo there cannot be two singleton sets of the same domain
        # there cannot be n identical sets of size n - 1, to extend this concept
        # how to handle subsets of these identical sets [(1), (1,2), (1,2)]
        # iterative reduction, remove the least occurent element from all sets until vec domain is empty which is
        # success, or a set is empty, which is a failure
        # problem here, bad set exhaustion
        for vec in board.vec_list:
            spot_domain_collection = list[set[int]]()
            combined_spot_domain = set[int]()
            for spot in vec.spot_members:
                if spot.commit == 0:
                    spot_domain_collection.append(spot.domain.copy())
                else:
                    continue
            combined_spot_domain.update(*spot_domain_collection)
            if not combined_spot_domain.issuperset(vec.domain):
                return False
            else:
                return True


    '''
    Building the reference grid in solve
    '''
    def build_grid(self) -> list[list[Spot_Ref]]:
        row_list: list[list[Spot_Ref]] = []
        for r in range(self.square_num):
            col_list: list[Spot_Ref] = []
            row_list.append(col_list)
            for c in range(self.square_num):
                b_spot: Spot_Ref = Spot_Ref(r, c, set[Vector_Ref]())
                col_list.append(b_spot)
        return row_list

    '''
    assign spot ref to vectors, and creating said vectors
    '''
    def build_vector_ref(self):
        for r in range(self.square_num):
            h_vec = set[Spot_Ref]()
            for c in range(self.square_num):
                h_vec.add(self.grid_ref[r][c])
            self.vec_set.add(Vector_Ref(h_vec))
        for c in range(self.square_num):
            v_vec = set[Spot_Ref]()
            for r in range(self.square_num):
                v_vec.add(self.grid_ref[r][c])
            self.vec_set.add(Vector_Ref(v_vec))
        """
        f_slash = set[Spot_Ref]()
        for r in range(self.square_num):
            f_slash.add(self.grid_ref[r][r])
        self.vec_set.add(Vector_Ref(f_slash))
        r_list = range(self.square_num)
        l_list = reversed(range(self.square_num))
        b_slash = set[Spot_Ref]()
        for r, c in zip(r_list, l_list):
            b_slash.add(self.grid_ref[r][c])
        self.vec_set.add(Vector_Ref(b_slash))
        """

        def get_square(row, col):
            square = set[Spot_Ref]()
            for sr in range(row, row + self.root_num):
                for sc in range(col, col + self.root_num):
                    square.add(self.grid_ref[sr][sc])
            return square

        for r in range(0, self.square_num, self.root_num):
            for c in range(0, self.square_num, self.root_num):
                self.vec_set.add(Vector_Ref(get_square(r, c)))

    def print_vec_list(self):
        for vec in self.vec_set:
            print(vec)

    '''
    In the references, link spots to vec
    '''
    def link_spot_to_vec(self):
        for vec in self.vec_set:
            for spot in vec.spot_list:
                spot.vectors.add(vec)

    '''
    build new board vectors using vector reference, and the new board copy
    '''
    def build_vector_sets(self, new_board: Board):
        for v_ref in self.vec_set:
            new_move_set = self.valid_moves.copy()
            new_vec = Vector(v_ref, new_move_set, set[Spot]())
            for s_ref in v_ref.spot_list:
                new_spot = new_board.grid[s_ref.row][s_ref.col]
                new_spot.vec_membership.add(new_vec)
                if new_spot.commit == 0:
                    continue
                else:
                    new_vec.domain.discard(new_spot.commit)
            new_board.vec_list.add(new_vec)

    '''
    find valid moves for each spot using vectors its in membership of
    '''
    def build_spot_domain(self, new_board: Board):
        for r in range(self.square_num):
            for c in range(self.square_num):
                new_spot = new_board.grid[r][c]
                for v in new_spot.vec_membership:
                    v.spot_members.add(new_spot)
                    if new_spot.commit == 0:
                        new_spot.domain = new_spot.domain.intersection(v.domain)

    '''
    Using references and new board, count the spots that arent committed. 
    '''
    def count_spot_constraints(self, new_board: Board):
        for r in range(self.square_num):
            for c in range(self.square_num):
                new_spot = new_board.grid[r][c]
                for v in new_spot.vec_membership:
                    for other_spot in v.spot_members:
                        if other_spot is new_spot:
                            continue
                        elif other_spot.commit == 0:
                            new_spot.constraints.add(other_spot)
                        else:
                            continue

    def copy_board(self, original: Board) -> Board:
        new = Board(self.square_num)
        for r in range(self.square_num):
            for c in range(self.square_num):
                new_spot = new.grid[r][c]
                new_spot.commit = original.grid[r][c].commit
                new_spot.r = r
                new_spot.c = c
                if new_spot.commit != 0:
                    new_spot.domain = None
                    continue
                else:
                    new_spot.domain = self.valid_moves.copy()
        return new

    @staticmethod
    def print_suite(board: Board):
        print()
        print("###########################")
        print(id(board))
        board.print_board()
        board.print_board_constraints()
        board.print_board_domain()
        print("############################")
        print()
        time.sleep(3)

    def solve(self, old_board: Board, move: tuple[int, int, int] = None):
        # if move is not None:
        #    old_board.print_spot(move)
        new_board = self.copy_board(old_board)
        self.process_board(new_board, move)
        if new_board.check_win():
            return True, new_board
        if not self.check_constraints(new_board):
            return False, None
        while len(new_board.queue) != 0:
            spot = new_board.pop_queue()
            spot_domain_list = list(spot.domain)  # to find the numbers that are most constrained
            sorted_domain_list = list[Element_sort]()
            for val in spot_domain_list:
                index = 0
                for i in range(len(new_board.element_list)):
                    if new_board.element_list[i].element == val:
                        index = i
                        break
                sorted_domain_list.append(Element_sort(val, index))
            sorted_domain_list.sort(key=lambda a: a.index)
            while len(sorted_domain_list) != 0:
                new_move = (spot.r, spot.c, sorted_domain_list.pop(0).element)
                win, board = self.solve(new_board, new_move)
                if win:
                    return True, board
                else:
                    continue
        return False, None


if __name__ == '__main__':
    set_1 = [(0, 2, 1), (0, 5, 2),
             (1, 2, 5), (1, 5, 6), (1, 7, 3),
             (2, 0, 4), (2, 1, 6), (2, 5, 5),
             (3, 3, 1), (3, 5, 4),
             (4, 0, 6), (4, 3, 8), (4, 6, 1), (4, 7, 4), (4, 8, 3),
             (5, 4, 9), (5, 6, 5), (5, 8, 8),
             (6, 0, 8), (6, 4, 4), (6, 5, 9), (6, 7, 5),
             (7, 0, 1), (7, 3, 3), (7, 4, 2),
             (8, 2, 9), (8, 6, 3)]

    set_2 = [(0, 2, 5), (0, 4, 1),
             (1, 2, 2), (1, 5, 4), (1, 7, 3),
             (2, 0, 1), (2, 2, 9), (2, 6, 2), (2, 8, 6),
             (3, 0, 2), (3, 4, 3),
             (4, 1, 4), (4, 6, 7),
             (5, 0, 5), (5, 5, 7), (5, 8, 1),
             (6, 3, 6), (6, 5, 3),
             (7, 1, 6), (7, 3, 1),
             (8, 4, 7), (8, 7, 5)]

    set_3 = [(0, 0, 6), (0, 1, 7),
             (1, 1, 2), (1, 2, 5),
             (2, 1, 9), (2, 3, 5), (2, 4, 6), (2, 6, 2),
             (3, 0, 3), (3, 4, 8), (3, 6, 9),
             (4, 6, 8), (4, 8, 1),
             (5, 3, 4), (5, 4, 7),
             (6, 2, 8), (6, 3, 6), (6, 7, 9),
             (7, 7, 1),
             (8, 0, 1), (8, 2, 6), (8, 4, 5), (8, 7, 7)]

    test_set_1 = [(0, 1, 3), (0, 2, 4),
                  (1, 0, 4), (1, 3, 2),
                  (2, 0, 1), (2, 3, 3),
                  (3, 1, 2), (3, 2, 1)]

    time1 = time.time()
    sodoku = Solver(root_num=3, start_info=set_2)
    print("START POSITION")
    sodoku.start_grid.print_board()
    # sodoku.start_grid.print_board_constraints()
    # sodoku.start_grid.print_board_domain()
    final_win, final_board = sodoku.solve(sodoku.start_grid)
    time2 = time.time()
    tdiff = time2 - time1
    print(tdiff)
    if final_board:
        print("Solution found")
        final_board.print_board()
    else:
        print("no solution")

    # todo look ahead would be useful
