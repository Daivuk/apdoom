
namespace APDoomLauncher
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.label1 = new System.Windows.Forms.Label();
            this.txtWidth = new System.Windows.Forms.TextBox();
            this.txtHeight = new System.Windows.Forms.TextBox();
            this.chkFullscreen = new System.Windows.Forms.CheckBox();
            this.btnLaunchDOOM = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnOverrideSettings = new System.Windows.Forms.Button();
            this.cboGame = new System.Windows.Forms.ComboBox();
            this.label8 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.txtCommandLine = new System.Windows.Forms.TextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.txtPassword = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.txtServer = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.txtPlayer = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.pictureBox4 = new System.Windows.Forms.PictureBox();
            this.pictureBox3 = new System.Windows.Forms.PictureBox();
            this.pictureBox2 = new System.Windows.Forms.PictureBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox4)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox3)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(48, 98);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(75, 17);
            this.label1.TabIndex = 1;
            this.label1.Text = "Resolution";
            // 
            // txtWidth
            // 
            this.txtWidth.Location = new System.Drawing.Point(197, 95);
            this.txtWidth.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.txtWidth.Name = "txtWidth";
            this.txtWidth.Size = new System.Drawing.Size(100, 22);
            this.txtWidth.TabIndex = 2;
            this.txtWidth.Text = "1280";
            // 
            // txtHeight
            // 
            this.txtHeight.Location = new System.Drawing.Point(303, 95);
            this.txtHeight.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.txtHeight.Name = "txtHeight";
            this.txtHeight.Size = new System.Drawing.Size(100, 22);
            this.txtHeight.TabIndex = 3;
            this.txtHeight.Text = "720";
            // 
            // chkFullscreen
            // 
            this.chkFullscreen.AutoSize = true;
            this.chkFullscreen.Location = new System.Drawing.Point(199, 68);
            this.chkFullscreen.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.chkFullscreen.Name = "chkFullscreen";
            this.chkFullscreen.Size = new System.Drawing.Size(95, 21);
            this.chkFullscreen.TabIndex = 1;
            this.chkFullscreen.Text = "Fullscreen";
            this.chkFullscreen.UseVisualStyleBackColor = true;
            this.chkFullscreen.CheckedChanged += new System.EventHandler(this.chkFullscreen_CheckedChanged);
            // 
            // btnLaunchDOOM
            // 
            this.btnLaunchDOOM.Location = new System.Drawing.Point(259, 697);
            this.btnLaunchDOOM.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.btnLaunchDOOM.Name = "btnLaunchDOOM";
            this.btnLaunchDOOM.Size = new System.Drawing.Size(149, 30);
            this.btnLaunchDOOM.TabIndex = 8;
            this.btnLaunchDOOM.Text = "Launch Game";
            this.btnLaunchDOOM.UseVisualStyleBackColor = true;
            this.btnLaunchDOOM.Click += new System.EventHandler(this.btnLaunchDOOM_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btnOverrideSettings);
            this.groupBox1.Controls.Add(this.cboGame);
            this.groupBox1.Controls.Add(this.label8);
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Controls.Add(this.txtCommandLine);
            this.groupBox1.Controls.Add(this.txtWidth);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.chkFullscreen);
            this.groupBox1.Controls.Add(this.txtHeight);
            this.groupBox1.Location = new System.Drawing.Point(101, 292);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.groupBox1.Size = new System.Drawing.Size(467, 211);
            this.groupBox1.TabIndex = 7;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Game Settings";
            // 
            // btnOverrideSettings
            // 
            this.btnOverrideSettings.Location = new System.Drawing.Point(197, 158);
            this.btnOverrideSettings.Name = "btnOverrideSettings";
            this.btnOverrideSettings.Size = new System.Drawing.Size(206, 30);
            this.btnOverrideSettings.TabIndex = 8;
            this.btnOverrideSettings.Text = "Advanced settings";
            this.btnOverrideSettings.UseVisualStyleBackColor = true;
            this.btnOverrideSettings.Click += new System.EventHandler(this.btnOverrideSettings_Click);
            // 
            // cboGame
            // 
            this.cboGame.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboGame.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cboGame.FormattingEnabled = true;
            this.cboGame.Items.AddRange(new object[] {
            "Ultimate DOOM",
            "DOOM II",
            "Heretic",
            "DOOM Open World"});
            this.cboGame.Location = new System.Drawing.Point(197, 39);
            this.cboGame.Name = "cboGame";
            this.cboGame.Size = new System.Drawing.Size(206, 24);
            this.cboGame.TabIndex = 0;
            this.cboGame.SelectedIndexChanged += new System.EventHandler(this.cboGame_SelectedIndexChanged);
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(48, 42);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(46, 17);
            this.label8.TabIndex = 7;
            this.label8.Text = "Game";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(48, 130);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(102, 17);
            this.label6.TabIndex = 6;
            this.label6.Text = "Command Line";
            // 
            // txtCommandLine
            // 
            this.txtCommandLine.Location = new System.Drawing.Point(197, 127);
            this.txtCommandLine.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.txtCommandLine.Name = "txtCommandLine";
            this.txtCommandLine.Size = new System.Drawing.Size(206, 22);
            this.txtCommandLine.TabIndex = 4;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.txtPassword);
            this.groupBox2.Controls.Add(this.label5);
            this.groupBox2.Controls.Add(this.txtServer);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.label4);
            this.groupBox2.Controls.Add(this.txtPlayer);
            this.groupBox2.Location = new System.Drawing.Point(101, 525);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.groupBox2.Size = new System.Drawing.Size(467, 148);
            this.groupBox2.TabIndex = 8;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Archipelago Settings";
            // 
            // txtPassword
            // 
            this.txtPassword.Location = new System.Drawing.Point(199, 95);
            this.txtPassword.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.txtPassword.Name = "txtPassword";
            this.txtPassword.PasswordChar = '*';
            this.txtPassword.Size = new System.Drawing.Size(204, 22);
            this.txtPassword.TabIndex = 7;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(48, 98);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(69, 17);
            this.label5.TabIndex = 5;
            this.label5.Text = "Password";
            // 
            // txtServer
            // 
            this.txtServer.Location = new System.Drawing.Point(199, 39);
            this.txtServer.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.txtServer.Name = "txtServer";
            this.txtServer.Size = new System.Drawing.Size(204, 22);
            this.txtServer.TabIndex = 5;
            this.txtServer.Text = "archipelago.gg:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(48, 42);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(50, 17);
            this.label3.TabIndex = 1;
            this.label3.Text = "Server";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(48, 70);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(73, 17);
            this.label4.TabIndex = 3;
            this.label4.Text = "Slot Name";
            // 
            // txtPlayer
            // 
            this.txtPlayer.Location = new System.Drawing.Point(199, 66);
            this.txtPlayer.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.txtPlayer.Name = "txtPlayer";
            this.txtPlayer.Size = new System.Drawing.Size(204, 22);
            this.txtPlayer.TabIndex = 6;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(12, 734);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(105, 17);
            this.label7.TabIndex = 9;
            this.label7.Text = "APDOOM 1.0.4";
            // 
            // pictureBox4
            // 
            this.pictureBox4.BackgroundImage = global::APDoomLauncher.Properties.Resources.Title4;
            this.pictureBox4.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.pictureBox4.Location = new System.Drawing.Point(88, 23);
            this.pictureBox4.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.pictureBox4.Name = "pictureBox4";
            this.pictureBox4.Size = new System.Drawing.Size(492, 240);
            this.pictureBox4.TabIndex = 12;
            this.pictureBox4.TabStop = false;
            // 
            // pictureBox3
            // 
            this.pictureBox3.BackgroundImage = global::APDoomLauncher.Properties.Resources.Title3;
            this.pictureBox3.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.pictureBox3.Location = new System.Drawing.Point(53, 28);
            this.pictureBox3.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.pictureBox3.Name = "pictureBox3";
            this.pictureBox3.Size = new System.Drawing.Size(575, 229);
            this.pictureBox3.TabIndex = 11;
            this.pictureBox3.TabStop = false;
            // 
            // pictureBox2
            // 
            this.pictureBox2.BackgroundImage = global::APDoomLauncher.Properties.Resources.Title2;
            this.pictureBox2.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.pictureBox2.Location = new System.Drawing.Point(88, 23);
            this.pictureBox2.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.pictureBox2.Name = "pictureBox2";
            this.pictureBox2.Size = new System.Drawing.Size(492, 240);
            this.pictureBox2.TabIndex = 10;
            this.pictureBox2.TabStop = false;
            this.pictureBox2.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBox2_Paint);
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackgroundImage = global::APDoomLauncher.Properties.Resources.Title;
            this.pictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.pictureBox1.Location = new System.Drawing.Point(88, 23);
            this.pictureBox1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(492, 240);
            this.pictureBox1.TabIndex = 0;
            this.pictureBox1.TabStop = false;
            this.pictureBox1.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBox1_Paint);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(671, 759);
            this.Controls.Add(this.pictureBox4);
            this.Controls.Add(this.pictureBox3);
            this.Controls.Add(this.pictureBox2);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnLaunchDOOM);
            this.Controls.Add(this.pictureBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Archipelago DOOM Launcher";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox4)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox3)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtWidth;
        private System.Windows.Forms.TextBox txtHeight;
        private System.Windows.Forms.CheckBox chkFullscreen;
        private System.Windows.Forms.Button btnLaunchDOOM;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.TextBox txtPassword;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtServer;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txtPlayer;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox txtCommandLine;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.ComboBox cboGame;
        private System.Windows.Forms.PictureBox pictureBox2;
        private System.Windows.Forms.PictureBox pictureBox3;
        private System.Windows.Forms.Button btnOverrideSettings;
        private System.Windows.Forms.PictureBox pictureBox4;
    }
}

