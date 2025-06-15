-- Create schema
CREATE SCHEMA ship_management;

-- Create User table
CREATE TABLE ship_management.users (
    id SERIAL PRIMARY KEY,
    firstname VARCHAR(40) NOT NULL,
    secondname VARCHAR(40) NOT NULL,
    thirdname VARCHAR(40) NOT NULL,
    description VARCHAR(200),
    password VARCHAR(256),
    role VARCHAR(8) CHECK (role IN ('Admiral', 'Sailor', 'Captain', 'None')) DEFAULT 'None'
);

-- Create Fleet table
CREATE TABLE ship_management.fleets (
    id SERIAL PRIMARY KEY,
    country VARCHAR(40) UNIQUE NOT NULL,
    description VARCHAR(200)
);

-- Create AdmiralDetails table
CREATE TABLE ship_management.admiral_details (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL UNIQUE REFERENCES ship_management.users(id) ON DELETE CASCADE,
    fleet_id INT UNIQUE REFERENCES ship_management.fleets(id) ON DELETE CASCADE
);

-- Create CaptainDetails table
CREATE TABLE ship_management.captain_details (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL UNIQUE REFERENCES ship_management.users(id) ON DELETE CASCADE,
    fleet_id INT UNIQUE REFERENCES ship_management.fleets(id) ON DELETE SET NULL
);

-- Create SailorDetails table
CREATE TABLE ship_management.sailor_details (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL UNIQUE REFERENCES ship_management.users(id) ON DELETE CASCADE,
    fleet_id INT UNIQUE REFERENCES ship_management.fleets(id) ON DELETE SET NULL
);


-- Create Ship table
CREATE TABLE ship_management.ships (
    id SERIAL PRIMARY KEY,
    name VARCHAR(40) NOT NULL
);

-- Create Mission table
CREATE TABLE ship_management.missions (
    id SERIAL PRIMARY KEY,
    description VARCHAR(200),
    is_complete BOOLEAN DEFAULT FALSE,
    ship_id INT REFERENCES ship_management.ships(id) ON DELETE CASCADE
);


-- Create the Request table
CREATE TABLE ship_management.requests (
    id SERIAL PRIMARY KEY, -- Unique identifier for the request
    role VARCHAR(8) NOT NULL CHECK (role IN ('Admiral', 'Sailor', 'Captain')), -- Role must exclude 'NONE'
    asker_id INT NOT NULL REFERENCES ship_management.users(id) ON DELETE CASCADE, -- Foreign key to the user making the request
    accepter_id INT NOT NULL REFERENCES ship_management.users(id) ON DELETE CASCADE -- Foreign key to the user accepting the request
);

CREATE TABLE ship_management.ship_sailors (
    ship_id INT REFERENCES ship_management.ships(id) ON DELETE CASCADE,
    sailor_details_id INT REFERENCES ship_management.sailor_details(id) ON DELETE CASCADE,
    PRIMARY KEY (ship_id, sailor_details_id)
);

CREATE TABLE ship_management.ship_fleet (
    id SERIAL PRIMARY KEY,
    ship_id INT UNIQUE NOT NULL REFERENCES ship_management.ships(id) ON DELETE CASCADE,
    fleet_id INT NOT NULL REFERENCES ship_management.fleets(id) ON DELETE CASCADE
);

CREATE TABLE ship_management.captains2ships (
    id SERIAL PRIMARY KEY,
    captain_details_id INT UNIQUE NOT NULL REFERENCES ship_management.captain_details(id) ON DELETE CASCADE,
    ship_id INT UNIQUE NOT NULL REFERENCES ship_management.ships(id) ON DELETE CASCADE
);

-- Additional constraints

-- Fleet must have only one admiral
ALTER TABLE ship_management.admiral_details
ADD CONSTRAINT one_admiral_per_fleet UNIQUE (fleet_id);

-- Ensure user role is consistent with other tables
-- AdmiralDetails role must be Admiral
CREATE OR REPLACE FUNCTION enforce_admiral_role()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT role FROM ship_management.users WHERE id = NEW.user_id) != 'Admiral' THEN
        RAISE EXCEPTION 'User must have role Admiral to be added to AdmiralDetails';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER verify_admiral_role
BEFORE INSERT OR UPDATE ON ship_management.admiral_details
FOR EACH ROW EXECUTE FUNCTION enforce_admiral_role();

-- CaptainDetails role must be Captain
CREATE OR REPLACE FUNCTION enforce_captain_role()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT role FROM ship_management.users WHERE id = NEW.user_id) != 'Captain' THEN
        RAISE EXCEPTION 'User must have role Captain to be added to CaptainDetails';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER verify_captain_role
BEFORE INSERT OR UPDATE ON ship_management.captain_details
FOR EACH ROW EXECUTE FUNCTION enforce_captain_role();

-- SailorDetails role must be Sailor
CREATE OR REPLACE FUNCTION enforce_sailor_role()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT role FROM ship_management.users WHERE id = NEW.user_id) != 'Sailor' THEN
        RAISE EXCEPTION 'User must have role Sailor to be added to SailorDetails';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER verify_sailor_role
BEFORE INSERT OR UPDATE ON ship_management.sailor_details
FOR EACH ROW EXECUTE FUNCTION enforce_sailor_role();

-- If a role is changed, clean up details tables
CREATE OR REPLACE FUNCTION clean_up_details_on_role_change()
RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM ship_management.admiral_details WHERE user_id = OLD.id;
    DELETE FROM ship_management.captain_details WHERE user_id = OLD.id;
    DELETE FROM ship_management.sailor_details WHERE user_id = OLD.id;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER clean_up_details
AFTER UPDATE OF role ON ship_management.users
FOR EACH ROW
WHEN (OLD.role != NEW.role)
EXECUTE FUNCTION clean_up_details_on_role_change();


CREATE OR REPLACE FUNCTION enforce_one_active_mission()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.is_complete = FALSE THEN
        IF EXISTS (
            SELECT 1 
            FROM ship_management.missions 
            WHERE ship_id = NEW.ship_id AND is_complete = FALSE
        ) THEN
            RAISE EXCEPTION 'A ship can only have one active mission at a time (is_complete = FALSE)';
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Ensure the function is properly terminated with a semicolon before creating the trigger
CREATE TRIGGER one_active_mission_per_ship
BEFORE INSERT OR UPDATE ON ship_management.missions
FOR EACH ROW
EXECUTE FUNCTION enforce_one_active_mission(); -- Fixed missing semicolon here

-- Function to update role details upon role change
CREATE OR REPLACE FUNCTION update_role_details()
RETURNS TRIGGER AS $$
BEGIN
    -- Remove old role details if role changes
    DELETE FROM ship_management.admiral_details WHERE user_id = OLD.id;
    DELETE FROM ship_management.captain_details WHERE user_id = OLD.id;
    DELETE FROM ship_management.sailor_details WHERE user_id = OLD.id;

    -- Insert new role details into appropriate table
    IF NEW.role = 'Admiral' THEN
        INSERT INTO ship_management.admiral_details (user_id) VALUES (NEW.id);
    ELSIF NEW.role = 'Captain' THEN
        INSERT INTO ship_management.captain_details (user_id) VALUES (NEW.id);
    ELSIF NEW.role = 'Sailor' THEN
        INSERT INTO ship_management.sailor_details (user_id) VALUES (NEW.id);
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Ensure trigger is created only after the function is defined
CREATE TRIGGER update_user_details_trigger
AFTER UPDATE OF role ON ship_management.users
FOR EACH ROW
WHEN (OLD.role IS DISTINCT FROM NEW.role)
EXECUTE FUNCTION update_role_details();


CREATE OR REPLACE FUNCTION set_default_role()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.role IS NULL OR TRIM(NEW.role) = '' THEN
        NEW.role := 'None';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Attach the trigger to the 'users' table
CREATE TRIGGER trigger_set_default_role
BEFORE INSERT ON ship_management.users
FOR EACH ROW EXECUTE FUNCTION set_default_role();


ALTER TABLE ship_management.users
ADD CONSTRAINT unique_user_names UNIQUE (firstname, secondname, thirdname);


ALTER TABLE ship_management.ship_fleet
ADD CONSTRAINT unique_ship_id UNIQUE (ship_id);

-- Ensure a captain can be assigned to only one ship
ALTER TABLE ship_management.captains2ships
ADD CONSTRAINT unique_captain UNIQUE (captain_details_id);

-- Ensure a ship can be assigned to only one captain
ALTER TABLE ship_management.captains2ships
ADD CONSTRAINT unique_ship UNIQUE (ship_id);