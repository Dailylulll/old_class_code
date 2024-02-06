import time


class Coord:
    row: int
    col: int

    def __init__(self, row: int, col: int):
        self.row = row  # vac loc
        self.col = col

    def __str__(self):
        return f"({self.row},{self.col})"

    def __repr__(self):
        return f"({self.row},{self.col})"

    def __eq__(self, other):
        return id(self) == id(other)

    def __hash__(self):
        return hash((self.col, self.row))


class Move:
    source: Coord
    dest: Coord
    score: float
    move: str

    def __init__(self, source: Coord, dest: Coord, cost: float, move: str):
        self.source = source
        self.dest = dest
        self.score = cost
        self.move = move

    def __str__(self) -> str:
        return f"Source: {self.source}, Dest: {self.dest}, Cost: {self.score}, Move: {self.move}"

    def __repr__(self) -> str:
        return f"Source: {self.source}, Dest: {self.dest}, Cost: {self.score}, Move: {self.move}"


class Board_Spot:
    coord: Coord
    moves: list[Move]

    def __str__(self) -> str:
        return f"({self.coord})"

    def moves_str(self) -> str:
        return f"({self.coord}, {self.moves})"

    def __init__(self, coord: Coord):
        self.coord = coord
        self.moves = list()

    def init_moves(self, board, row: int, col: int) -> None:
        dest_coord = board.get_board_spot_cord(self.coord.row, self.coord.col)
        self.moves.append(Move(self.coord, dest_coord, .6, "suck"))
        if self.coord.row > 0:
            dest_coord = board.get_board_spot_cord(self.coord.row - 1, self.coord.col)
            self.moves.append(Move(self.coord, dest_coord, .8, "up"))
        if self.coord.row < board.row - 1:
            dest_coord = board.get_board_spot_cord(self.coord.row + 1, self.coord.col)
            self.moves.append(Move(self.coord, dest_coord, .7, "down"))
        if self.coord.col > 0:
            dest_coord = board.get_board_spot_cord(self.coord.row, self.coord.col - 1)
            self.moves.append(Move(self.coord, dest_coord, 1, "left"))
        if self.coord.col < board.col - 1:
            dest_coord = board.get_board_spot_cord(self.coord.row, self.coord.col + 1)
            self.moves.append(Move(self.coord, dest_coord, .9, "right"))


class Edge:
    source: Coord
    dest: Coord
    score: float
    parents: str
    dirty_complete: list[Coord]
    move: str
    dirt_remaining: int

    def __init__(self, source: Coord, dest: Coord, score: float, parents: str, dirty_complete: list[Coord],
                 move: str, dirt_remaining: int):
        self.source = source
        self.dest = dest
        self.score = score  # total score
        self.parents = parents
        self.dirty_complete = dirty_complete.copy()  # These should be references to the board dirty list
        self.dirt_remaining = dirt_remaining
        self.move = move

    def __str__(self) -> str:
        return f"S:{str(self.source)}, D:{str(self.dest)}, {self.score}, {' '.join(str(x) for x in self.dirty_complete)}, {self.move}"

    def __repr__(self) -> str:
        return f"({str(self.source)}, {self.score}, {' '.join(str(x) for x in self.dirty_complete)}, {self.move})"


class Board:
    row: int
    col: int
    grid: list[list[Board_Spot]]
    dirt_list: list[Coord]
    total_dirt: int

    def __init__(self, list_of_dirt: list[list[int]], row: int, col: int):
        self.row = row
        self.col = col
        self.grid = self.build_board(row, col)
        self.init_moves()
        self.dirt_list = self.build_dirt_list(list_of_dirt)
        self.total_dirt = len(list_of_dirt)

    def __str__(self) -> str:
        string: str = ""
        for col in self.grid:
            for spot in col:
                string += spot.__str__() + " "
            string += "\n"
        return string

    def get_board_spot_cord(self, r: int, c: int) -> Coord:
        return self.grid[r][c].coord

    def get_board_spot_moves(self, coord: Coord) -> list[Move]:
        return self.grid[coord.row][coord.col].moves

    def build_dirt_list(self, dirt_list: list[list[int]]) -> list[Coord]:
        coord_list = []
        for i in dirt_list:
            coord_list.append(self.get_board_spot_cord(i[0], i[1]))
        return coord_list

    def build_board(self, row: int, col: int) -> list[list[Board_Spot]]:
        row_list: list[list[Board_Spot]] = []
        for r in range(row):
            col_list: list[Board_Spot] = []
            row_list.append(col_list)
            for c in range(col):
                coord: Coord = Coord(r, c)
                col_list.append(Board_Spot(coord))
        return row_list

    def init_moves(self):
        for r in range(self.row):
            for c in range(self.col):
                self.grid[r][c].init_moves(self, r, c)


class GUCTS:
    fringe: list[Edge]
    state_map: dict[frozenset]
    board: Board
    dirt_count: int
    start: Coord
    expanded_nodes: int
    generated_nodes: int
    hold_coord: Coord = Coord(-1,-1)

    def __init__(self, dirty_list: list[list[int]], start_spot: Coord, row: int =4, col: int =5):
        self.board = Board(dirty_list, row, col)
        self.dirt_count = len(dirty_list)
        self.start = self.board.get_board_spot_cord(start_spot.row, start_spot.col)
        self.fringe = list[Edge]()
        self.state_map = dict[frozenset]()
        self.generated_nodes = 1
        self.expanded_nodes = 0

    def return_frozenset(self, edge: Edge):
        set_tuple: tuple[str, Coord]
        # print(edge)
        tuple_list: list[tuple[str, Coord]] = list[tuple[str, Coord]]()
        set_tuple = ("loc", edge.dest)
        tuple_list.append(set_tuple)
        for i in range(self.dirt_count):
            try:
                set_tuple = ('suc', edge.dirty_complete[i])
                tuple_list.append(set_tuple)
            except IndexError:
                break
        return frozenset(tuple_list)

    def sort(self, edge: Edge) -> float:
        return edge.score

    def solve(self):
        start_time = time.time()
        start_edge: Edge = Edge(source=self.start, dest=self.start, score=0, parents='', dirty_complete=[], move="start",
                                dirt_remaining=self.board.total_dirt)
        start_edge.parents += str(start_edge)
        self.fringe.append(start_edge)
        f_set = self.return_frozenset(start_edge)
        self.state_map[f_set] = True
        # print(hash(f_set))
        while time.time() - start_time < 3600:
            pop_edge: Edge
            try:
                pop_edge = self.fringe.pop(0)
            except IndexError:
                return time.time() - start_time, "No solution, fringe ran out of nodes", 0.0
            try:
                1 // pop_edge.dirt_remaining
            except ZeroDivisionError:
                return time.time() - start_time, pop_edge.parents, pop_edge.score
            expand_list: list[Edge] = self.expand(pop_edge)
            self.generated_nodes += len(expand_list)
            self.fringe.extend(expand_list)
            self.fringe.sort(key=lambda edge: (edge.score, edge.dest.row, edge.dest.col))
        return time.time() - start_time, f"One hour has passed, no solution generated", 0.0

    # State = location and dirt list
    # before expanding do suck operation first, and clean dirt
    # before expanding a node, compress it to a state
    # if that state is in the closed set do not expand

    def expand(self, p_edge: Edge) -> list[Edge]:
        self.expanded_nodes += 1
        move_list: list[Move] = self.board.get_board_spot_moves(p_edge.dest)
        expand_list: list[Edge] = list()
        # print(f"popped edge: {p_edge}")
        # print(move_list)
        for move in move_list:
            clean_list: list[Coord] = p_edge.dirty_complete.copy()
            # The new edge represents the state after the edge is completed
            new_edge: Edge = Edge(source=move.source, dest=move.dest, score=round(move.score + p_edge.score, 1),
                             parents=p_edge.parents, dirty_complete=clean_list, move=move.move,
                             dirt_remaining=p_edge.dirt_remaining)
            if new_edge.move == "suck" and new_edge.dest in self.board.dirt_list and new_edge.dest not in new_edge.dirty_complete:
                new_edge.dirty_complete.append(new_edge.dest)
                new_edge.dirt_remaining -= 1
            f_set = self.return_frozenset(new_edge)
            # print(f"Examined edge {new_edge}")
            if f_set in self.state_map:
                # print("denied")
                continue
            else:
                # print("pushed")
                self.state_map[f_set] = True
            new_edge.parents += f"\n{str(new_edge)}"
            expand_list.append(new_edge)
        return expand_list


if __name__ == "__main__":

    dirt_list = [[0, 1], [1, 3], [2, 4]]
    start_loc = Coord(1, 1)
    search = GUCTS(dirt_list, start_loc, row=4, col=5)
    '''
    dirt_list = [[0, 1], [1, 0], [1, 3], [2, 2]]
    start_loc = Coord(2, 1)
    search = GUCTS(dirt_list, start_loc, row=4, col=5)
    '''
    sol: str
    score: float
    time, sol, score = search.solve()
    print(search.board)
    print(f"dirt_list: {dirt_list}")
    print(f"start loc {start_loc}")
    print(f"Solution:\n{sol}")
    print(f"Score: {score}")
    print(f"Expanded nodes: {search.expanded_nodes}")
    print(f"Generated nodes: {search.generated_nodes}")
    print(f"Time: {time}")
    print(f"Closed set: {search.state_map}")
