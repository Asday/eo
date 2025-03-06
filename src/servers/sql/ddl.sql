CREATE TABLE public.launcher (
	id uuid DEFAULT gen_random_uuid() NOT NULL,
	ip inet NOT NULL,
	port int4 NOT NULL,
	heartbeat timestamp NOT NULL,
	CONSTRAINT launcher_pk PRIMARY KEY (id)
);
