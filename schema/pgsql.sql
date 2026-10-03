-- gsql schema for simple-dnsd
CREATE TABLE IF NOT EXISTS domains (
  id SERIAL PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  master VARCHAR(128) DEFAULT NULL,
  last_check INT DEFAULT NULL,
  type VARCHAR(8) NOT NULL,
  notified_serial INT DEFAULT NULL,
  account VARCHAR(40) DEFAULT NULL
);
CREATE TABLE IF NOT EXISTS records (
  id SERIAL PRIMARY KEY,
  domain_id INT DEFAULT NULL,
  name VARCHAR(255) DEFAULT NULL,
  type VARCHAR(10) DEFAULT NULL,
  content VARCHAR(65535) DEFAULT NULL,
  ttl INT DEFAULT NULL,
  prio INT DEFAULT NULL,
  disabled BOOL DEFAULT 'f',
  ordername VARCHAR(255),
  auth BOOL DEFAULT 't'
);
CREATE TABLE IF NOT EXISTS comments (
  id SERIAL PRIMARY KEY,
  domain_id INT NOT NULL,
  name VARCHAR(255) NOT NULL,
  type VARCHAR(10) NOT NULL,
  modified_at INT NOT NULL,
  account VARCHAR(40) DEFAULT NULL,
  comment VARCHAR(65535) NOT NULL
);
CREATE TABLE IF NOT EXISTS domainmetadata (
  id SERIAL PRIMARY KEY,
  domain_id INT NOT NULL,
  kind VARCHAR(32),
  content TEXT
);
CREATE TABLE IF NOT EXISTS cryptokeys (
  id SERIAL PRIMARY KEY,
  domain_id INT NOT NULL,
  flags INT NOT NULL,
  active BOOL,
  published BOOL DEFAULT 't',
  content TEXT
);
CREATE TABLE IF NOT EXISTS tsigkeys (
  id SERIAL PRIMARY KEY,
  name VARCHAR(255),
  algorithm VARCHAR(50),
  secret VARCHAR(255)
);
