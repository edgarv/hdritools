// Internal definitions for RGBE files IO, which should not be known outside

#if !defined(RGBEIOPRIVATE_H)
#define RGBEIOPRIVATE_H

namespace pcg {

	// The is stuff which won't be defined here because it uses a lot of templates
	namespace rgbeio_internal {

		// Structure which represents the header of the RGBE file
		struct rgbe_header_info {

		private:
			/* flags indicating which fields in an rgbe_header_info are valid */
			const static unsigned int Valid_ProgramType = 0x01;
			const static unsigned int Valid_Gamma       = 0x02;
			const static unsigned int Valid_Exposure    = 0x04;

		public:

			unsigned int valid;		/* indicate which fields are valid */
			char programtype[16];	/* listed at beginning of file to identify it 
									* after "#?".  defaults to "RGBE" */ 
			float gamma;			/* image has already been gamma corrected with 
									* given gamma.  defaults to 1.0 (no correction) */
			float exposure;			/* a value of 1.0 in an image corresponds to
									* <exposure> watts/steradian/m^2. 
									* defaults to 1.0 */

			// Basic simple constructor: No field is valid by default
			rgbe_header_info() {

				valid = 0;
				programtype[0] = 0;
				gamma = exposure = 1.0f;
			}

			// Utility methods for the header

			// Does the header have a valid program?
			inline bool isValidProgramType() const {
				return (valid & Valid_ProgramType) != 0;
			}

			// Does the header have a valid gamma?
			inline bool isValidGamma() const {
				return (valid & Valid_Gamma) != 0;
			}

			// Does the header have a valid exposure?
			inline bool isValidExposure() const {
				return (valid & Valid_Exposure) != 0;
			}

			// Sets or unset the valid program type flag
			inline void setValidProgramType(const bool isValid) {
				if (isValid)
					valid |= Valid_ProgramType;
				else
					valid &= ~Valid_ProgramType;
			}

			// Sets or unset the valid gamma flag
			inline void setValidGamma(const bool isValid) {
				if (isValid)
					valid |= Valid_Gamma;
				else
					valid &= ~Valid_Gamma;
			}

			// Sets or unset the valid exposure flag
			inline void setValidExposure(const bool isValid) {
				if (isValid)
					valid |= Valid_Exposure;
				else
					valid &= ~Valid_Exposure;
			}
		};

		/* return codes for rgbe routines */
		const static int RGBE_RETURN_SUCCESS =  0;
		const static int RGBE_RETURN_FAILURE = -1;

		enum rgbe_error_codes {
		  rgbe_read_error,
		  rgbe_write_error,
		  rgbe_format_error,
		  rgbe_memory_error,
		};


		/* default error routine.  change this to change error handling */
		int rgbe_error(rgbe_error_codes code, const char *msg);


		// ####################################################################
		// ####         READING         #######################################
		// ####################################################################

		/* minimal header reading.  modify if you want to parse more information */
		int readHeader(istream &is, int &width, int &height, rgbe_header_info &info);

		// ####################################################################
		// ####         WRITING         #######################################
		// ####################################################################


		/* default minimal header. modify if you want more information in header */
		int writeHeader(ostream &os, int width, int height,
							const rgbe_header_info &info);


		/* The code below is only needed for the run-length encoded files. */
		/* Run length encoding adds considerable complexity but does */
		/* save some space.  For each scanline, each channel (r,g,b,e) is */
		/* encoded separately for better compression. */
		int writeBytes_RLE(ostream &os, const unsigned char *data, int numbytes);

	}

}


#endif /* RGBEIOPRIVATE_H */
